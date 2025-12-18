/************************** DISCLAIMER ******************************
 * The following code is taken from the example project attached to *
 * the assignment.                                                  *
 ********************************************************************/
#include "tlc5947.h"

#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_rom_sys.h"

#define TLC5947_BITS_PER_CH 12
#define TLC5947_CH_PER_CHIP 24
#define TLC5947_BITS_PER_CHIP \
  (TLC5947_BITS_PER_CH * TLC5947_CH_PER_CHIP)               // 288
#define TLC5947_BYTES_PER_CHIP (TLC5947_BITS_PER_CHIP / 8)  // 36

static void pack_frame_msbfirst(const tlc5947_t *dev) {
  memset(dev->tx, 0, dev->frame_bytes);

  size_t bit_idx =
      0;  // index bitu v dev->tx (MSB-first na drátu v rámci bajtu)
  for (int chip = dev->chips - 1; chip >= 0; --chip) {
    const int base = chip * TLC5947_CH_PER_CHIP;  // 24 kanálů na čip
    for (int ch = TLC5947_CH_PER_CHIP; ch > 0; --ch) {
      const uint16_t v =
          dev->gs[base + (ch - 1)] & 0x0FFFu;  // 12bit hodnota kanálu

      // MSB -> LSB (bity 11..0)
      for (int b = 11; b >= 0; --b) {
        if (v & (1u << b)) {
          const size_t byte = bit_idx >> 3;
          const int bit =
              7 - (bit_idx & 7);  // SPI posílá MSB bitu bajtu jako první
          dev->tx[byte] |= (uint8_t)(1u << bit);
        }
        ++bit_idx;
      }
    }
  }
}

esp_err_t tlc5947_init(tlc5947_t *dev, const tlc5947_config_t *cfg) {
  ESP_RETURN_ON_FALSE(dev && cfg, ESP_ERR_INVALID_ARG, "tlc5947", "null arg");
  memset(dev, 0, sizeof(*dev));
  dev->chips = cfg->chips;
  dev->channels = cfg->chips * TLC5947_CH_PER_CHIP;
  dev->frame_bytes = cfg->chips * TLC5947_BYTES_PER_CHIP;
  dev->xlat_io = cfg->xlat_io;
  dev->blank_io = cfg->blank_io;

  // Buffers
  dev->gs = (uint16_t *)calloc(dev->channels, sizeof(uint16_t));
  dev->tx = (uint8_t *)calloc(dev->frame_bytes, 1);
  ESP_RETURN_ON_FALSE(dev->gs && dev->tx, ESP_ERR_NO_MEM, "tlc5947", "alloc");

  // SPI bus/device
  spi_bus_config_t bus = {
      .mosi_io_num = cfg->mosi_io,
      .miso_io_num = -1,
      .sclk_io_num = cfg->sclk_io,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = (int)dev->frame_bytes,
      .flags = SPICOMMON_BUSFLAG_IOMUX_PINS  // use if on default pins; harmless
                                             // otherwise
  };
  // Bus init (ok i když už existuje – vrátí ESP_ERR_INVALID_STATE)
  esp_err_t err = spi_bus_initialize(
      cfg->host, &bus, cfg->dma_chan ? cfg->dma_chan : SPI_DMA_CH_AUTO);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

  spi_device_interface_config_t ifc = {
      .clock_speed_hz = cfg->clock_hz > 0 ? cfg->clock_hz : 10 * 1000 * 1000,
      .mode = 0,           // CPOL=0, CPHA=0 (data latch on rising edge)
      .spics_io_num = -1,  // no CS pin on TLC5947
      .queue_size = 1,
      .flags = SPI_DEVICE_NO_DUMMY};
  spi_bus_add_device(cfg->host, &ifc, &dev->spi);

  // GPIOs
  gpio_config_t io = {
      .pin_bit_mask = (1ULL << dev->xlat_io) | (1ULL << dev->blank_io),
      .mode = GPIO_MODE_OUTPUT,
      .pull_down_en = 0,
      .pull_up_en = 0};
  gpio_config(&io);
  // Bezpečný start: drž BLANK=1 (vše off), XLAT=0
  gpio_set_level(dev->xlat_io, 0);
  gpio_set_level(dev->blank_io, 1);

  // Nulový rámec → latch → povolit výstupy
  tlc5947_fill(dev, 0);
  tlc5947_update(dev, true);
  return ESP_OK;
}

void tlc5947_deinit(tlc5947_t *dev) {
  if (!dev) return;
  if (dev->spi) {
    spi_device_handle_t h = dev->spi;
    dev->spi = NULL;
    spi_bus_remove_device(h);
  }
  if (dev->tx) {
    free(dev->tx);
    dev->tx = NULL;
  }
  if (dev->gs) {
    free(dev->gs);
    dev->gs = NULL;
  }
}

void tlc5947_fill(tlc5947_t *dev, uint16_t value) {
  for (int i = 0; i < dev->channels; ++i) dev->gs[i] = (value & 0x0FFFu);
}

uint16_t tlc5947_u8_to_u12_gamma(uint8_t x) {
  // jednoduchá integer approx gamma 2.2 → 12bit
  // výsledek v rozsahu 0..4095
  // (x/255)^2.2 * 4095 ≈ exp(2.2*ln(x/255))*4095 ; tady použitá LUT-less
  // approximace pro embedded jednodušeji: (x*x*x) s přepočtem
  uint32_t t = (uint32_t)x;
  uint32_t y = (t * t * t) / (255u * 255u);  // ~gamma 3.0; pro LED bývá OK
  if (y > 4095u) y = 4095u;
  return (uint16_t)y;
}

esp_err_t tlc5947_update(tlc5947_t *dev, bool vblank_sync) {
  pack_frame_msbfirst(dev);

  spi_transaction_t t = {.length = dev->frame_bytes * 8, .tx_buffer = dev->tx};
  spi_device_transmit(dev->spi, &t);

  if (vblank_sync) {
    // BLANK high → XLAT pulse → BLANK low
    gpio_set_level(dev->blank_io, 1);
    gpio_set_level(dev->xlat_io, 1);
    // datasheet povoluje SCLK až 100 ns po XLAT↑ – 1 us je pohodlná rezerva
    esp_rom_delay_us(1);
    gpio_set_level(dev->xlat_io, 0);
    gpio_set_level(dev->blank_io, 0);
  } else {
    // Rychlé latnutí – krátký „black frame“ během přepnutí je možný
    gpio_set_level(dev->xlat_io, 1);
    esp_rom_delay_us(1);
    gpio_set_level(dev->xlat_io, 0);
  }
  return ESP_OK;
}

void tlc5947_set_blank(const tlc5947_t *dev, bool blank) {
  gpio_set_level(dev->blank_io, blank ? 1 : 0);
}

/********************************EOF tlc5947.c*********************************/