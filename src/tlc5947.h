#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// Počet řádků panelu (odpovídá počtu RGB trojic v jednom TLC5947)
#define ROWS 8
#define COLS 16

// Tyto tabulky definují mapování OUTx → barevná složka
// Implementace (definice obsahu) je v main.c
extern const uint8_t MAP_R[ROWS];
extern const uint8_t MAP_G[ROWS];
extern const uint8_t MAP_B[ROWS];

typedef struct {
    spi_device_handle_t spi;
    gpio_num_t xlat_io;   // latch (XLAT)
    gpio_num_t blank_io;  // output enable / BLANK (active high)
    int chips;            // number of TLC5947 in chain
    int channels;         // chips * 24
    size_t frame_bytes;   // 36 * chips
    uint16_t *gs;         // [channels] 12-bit values
    uint8_t  *tx;         // packed bitstream [frame_bytes]
} tlc5947_t;

typedef struct {
    spi_host_device_t host;     // SPI2_HOST / SPI3_HOST / SPI1_HOST (SoC-dependent)
    int mosi_io;                // connected to SIN
    int sclk_io;                // connected to SCLK
    gpio_num_t xlat_io;         // XLAT (latch)
    gpio_num_t blank_io;        // BLANK (OE, active-high)
    int chips;                  // number of cascaded TLC5947
    int clock_hz;               // SPI clock (e.g. 10 MHz)
    int dma_chan;               // 0=auto, 1/2=manual
} tlc5947_config_t;

/** Create+init driver (allocates buffers). */
esp_err_t tlc5947_init(tlc5947_t *dev, const tlc5947_config_t *cfg);

/** Deinit + free buffers (does NOT remove SPI bus). */
void tlc5947_deinit(tlc5947_t *dev);

/** Set raw 12-bit value of a channel (0..chips*24-1). */
static inline void tlc5947_set_ch(tlc5947_t *dev, int ch, uint16_t value) {
    if ((unsigned)ch < (unsigned)dev->channels) dev->gs[ch] = (value & 0x0FFFu);
}

/** Get raw 12-bit value. */
static inline uint16_t tlc5947_get_ch(const tlc5947_t *dev, int ch) {
    return (unsigned)ch < (unsigned)dev->channels ? (dev->gs[ch] & 0x0FFFu) : 0;
}

/** Fill all channels with the same 12-bit value. */
void tlc5947_fill(tlc5947_t *dev, uint16_t value);

/** Convert 8-bit to 12-bit s gamma≈2.2 (optional helper). */
uint16_t tlc5947_u8_to_u12_gamma(uint8_t x);

/** Push current buffer to TLC5947 chain over SPI and latch.
 *  If vblank_sync=true: BLANK↑ → XLAT↑ → (short delay) → BLANK↓.
 *  If false: pouze XLAT↑ (rychlejší, může krátce bliknout).
 */
esp_err_t tlc5947_update(tlc5947_t *dev, bool vblank_sync);

/** Force BLANK level (true=outputs off). */
void tlc5947_set_blank(const tlc5947_t *dev, bool blank);

#ifdef __cplusplus
}
#endif