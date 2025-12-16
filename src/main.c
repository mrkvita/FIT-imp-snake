#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tlc5947.h"

typedef enum {
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT,
  DIR_EMPTY,
} Direction;

typedef struct {
  int r;
  int c;
} Pos;

typedef struct {
  Pos pos;
  Direction name;
  Direction opposite;
} Dir;

typedef struct {
  Pos body[ROWS * COLS];
  size_t len;
  Dir dir;
} Snake;

#define QUEUE_SIZE 5
typedef struct {
  Direction q[QUEUE_SIZE];  // buffer 5 last directions
  size_t head;
  size_t tail;
  size_t occupied;
} Queue;

void queue_push(Queue *queue, Direction dir) {
  if (queue->occupied >= QUEUE_SIZE) {
    return;  // Queue is full, do not add new direction
  }
  queue->q[queue->tail] = dir;
  queue->tail = (queue->tail + 1) % QUEUE_SIZE;
  queue->occupied++;
}

void queue_pop(Queue *queue, Direction *dir) {
  if (queue->occupied == 0) {
    return;
  }
  *dir = queue->q[queue->head];
  queue->head = (queue->head + 1) % QUEUE_SIZE;
  queue->occupied--;
}

void queue_peek(Queue *queue, Direction *dir) {
  if (queue->occupied == 0) {
    return;
  }
  *dir = queue->q[queue->head];
}

void queue_peek_last(Queue *queue, Direction *dir) {
  if (queue->occupied == 0) {
    return;
  }
  size_t last_index = (queue->tail + QUEUE_SIZE - 1) % QUEUE_SIZE;
  *dir = queue->q[last_index];
}

// Delta pro kazdy smer, index je enum
static const Dir DIR_DELTA[4] = {[DIR_UP] = {{-1, 0}, DIR_UP, DIR_DOWN},
                                 [DIR_DOWN] = {{1, 0}, DIR_DOWN, DIR_UP},
                                 [DIR_LEFT] = {{0, -1}, DIR_LEFT, DIR_RIGHT},
                                 [DIR_RIGHT] = {{0, 1}, DIR_RIGHT, DIR_LEFT}};

static Queue direction = {.q = {}, .head = 0, .tail = 0};

static Snake snake = {.body =
                          {
                              {ROWS / 2, COLS / 2 + 3},
                              {ROWS / 2, COLS / 2 + 2},
                              {ROWS / 2, COLS / 2 + 1},
                              {ROWS / 2, COLS / 2},
                          },
                      .len = 4,
                      .dir = DIR_DELTA[DIR_RIGHT]};

// ==== PINY 74HCT154 ====
// Adresové vstupy: A = ADDR0, B = ADDR1, C = ADDR2, D = ADDR3
#define HCT154_ADDR0 25
#define HCT154_ADDR1 17
#define HCT154_ADDR2 16
#define HCT154_ADDR3 27
// COL_EN = společné povolení výstupů (invertované řízení):
//   HIGH = všechny sloupce vypnuté
//   LOW  = dekodér aktivní, vybraný sloupec povolen
#define HCT154_COL_EN 14

// Tla4č
#define HTC154_SW1 22
#define HTC154_SW2 21
#define HTC154_SW3 26
#define HTC154_SW4 4

// ==== TLC5947 PINY ====
#define TLC_MOSI 23
#define TLC_SCLK 18
#define TLC_XLAT 13
#define TLC_BLANK 12

// Cílová frame rate a odvozené časy
#define FRAME_RATE_HZ 60
#define GAME_RATE_HZ 5
#define COL_DWELL_US (1000000 / (FRAME_RATE_HZ * COLS))

// --- 16bit framebuffer (hodnoty 0..4095) ---
typedef struct {
  uint16_t r, g, b;
} rgb16_t;
static rgb16_t fb[ROWS][COLS];
static rgb16_t fb0[ROWS][COLS];  // baseline kopie pro obnovení

// Ovladač TLC
static tlc5947_t tlc;

// Stav multiplexu
static volatile int cur_col = -1;

// --- Animace ---
typedef enum { ANIM_WIPE_IN = 0, ANIM_ROW_CLEAR = 1 } anim_state_t;
static volatile anim_state_t anim = 3;

static volatile int max_lit_cols = COLS;  // pro wipe-in
static volatile int wipe_dir =
    +1;  // (zde zůstává +1, používáme jen pro wipe-in)
static volatile uint8_t frame_cnt = 0;  // zpomalení kroků
#define Wipe_Slowdown_Frames 3  // každé 3 frame-ticky posuň hranici sloupců

static volatile int row_to_clear = 0;  // pro mazání řádků
static volatile uint8_t clear_cnt = 0;
#define CLEAR_ROW_HOLD_FRAMES 5  // prodleva mezi řádky (5×20 ms = ~100 ms)

// ====== MAPOVÁNÍ KANÁLŮ TLC5947 ======
// R: 1, 4, 7, 10, 13, 16, 19, 22
// G: 0, 3, 6, 9, 12, 15, 18, 21
// B: 2, 5, 8, 11, 14, 17, 20, 23
const uint8_t MAP_R[ROWS] = {1, 4, 7, 10, 13, 16, 19, 22};
const uint8_t MAP_G[ROWS] = {0, 3, 6, 9, 12, 15, 18, 21};
const uint8_t MAP_B[ROWS] = {2, 5, 8, 11, 14, 17, 20, 23};

static inline int ch_r(int row) { return MAP_R[row & 7]; }
static inline int ch_g(int row) { return MAP_G[row & 7]; }
static inline int ch_b(int row) { return MAP_B[row & 7]; }

// Flag pro stisk tlačítka
static bool button_pressed = false;

bool conflictDir(Direction d, Direction last) {
  if (last == DIR_EMPTY) {
    if (snake.dir.name == d ||
        snake.dir.opposite == d) {  // conflict with snake
      return true;
    }
  }
  if (last != DIR_EMPTY) {
    if (d == last || d == DIR_DELTA[last].opposite) {
      return true;
    }
  }
  return false;
}

void insert_dir(Direction dir) {  // only inserts the direction if allowed
  Direction last_dir = DIR_EMPTY;
  queue_peek_last(&direction, &last_dir);
  if (!conflictDir(dir, last_dir)) {
    queue_push(&direction, dir);
  }
}
Direction test = DIR_EMPTY;
// === Interrupt handler pro tlačítko ===
static void IRAM_ATTR button_isr_handler(void *arg) {
  /* Handle debouncing and spamming  */
  static int64_t last_press = 0;
  int64_t now = esp_timer_get_time() / 1000;  // in ms
  if (now - last_press < 50) {
    return;
  }
  last_press = now;

  button_pressed = true;

  Direction dir = (Direction)(uintptr_t)arg;
  insert_dir(dir);
  test = dir;
}

// === Pomocné: GPIO 74HCT154 ===
static inline void col_disable_all(void) { gpio_set_level(HCT154_COL_EN, 1); }
static inline void col_enable_selected(void) {
  gpio_set_level(HCT154_COL_EN, 0);
}
static inline void col_select(int col) {
  gpio_set_level(HCT154_ADDR0, (col >> 0) & 1);
  gpio_set_level(HCT154_ADDR1, (col >> 1) & 1);
  gpio_set_level(HCT154_ADDR2, (col >> 2) & 1);
  gpio_set_level(HCT154_ADDR3, (col >> 3) & 1);
}

// Naplní TLC hodnotami pro daný sloupec; 'lit'==false sloupec zhasne
static void load_column_into_tlc(int col, bool lit) {
  for (int r = 0; r < ROWS; ++r) {
    rgb16_t px = fb[r][col];
    // držíme 0..4095; horní bity odmaskujeme pro jistotu
    uint16_t rv = lit ? (px.r & 0x0FFF) : 0;
    uint16_t gv = lit ? (px.g & 0x0FFF) : 0;
    uint16_t bv = lit ? (px.b & 0x0FFF) : 0;
    tlc5947_set_ch(&tlc, ch_r(r), rv);
    tlc5947_set_ch(&tlc, ch_g(r), gv);
    tlc5947_set_ch(&tlc, ch_b(r), bv);
  }
}

// Periodický multiplex (každých COL_DWELL_US)
static void IRAM_ATTR scan_timer_cb(void *arg) {
  col_disable_all();  // během latche nic nesvítí
  cur_col = (cur_col + 1) % COLS;

  bool lit = true;
  // ve fázi WIPE_IN svítí jen prvních max_lit_cols sloupců
  if (anim == ANIM_WIPE_IN) lit = (cur_col < max_lit_cols);

  load_column_into_tlc(cur_col, lit);
  tlc5947_update(&tlc, true);  // BLANK-sync latch

  col_select(cur_col);
  col_enable_selected();
}

// „Wipe“ + „row clear“ režie – běží 50 Hz
static void IRAM_ATTR frame_timer_cb(void *arg) {
  // delete tail
  fb[snake.body[snake.len - 1].r][snake.body[snake.len - 1].c] =
      (rgb16_t){0, 0, 0};

  // moved body
  for (int i = snake.len - 1; i > 0; i--) {
    snake.body[i] = snake.body[i - 1];
  }
  Direction next_dir = snake.dir.name;
  queue_pop(&direction, &next_dir);  // invariant if empty
  snake.dir = DIR_DELTA[next_dir];

  // new head
  snake.body[0].r += snake.dir.pos.r;
  snake.body[0].c += snake.dir.pos.c;
  snake.body[0].r = (snake.body[0].r + ROWS) % ROWS;
  snake.body[0].c = (snake.body[0].c + COLS) % COLS;

  // draw snake
  for (int i = 0; i < snake.len; i++) {
    fb[snake.body[i].r][snake.body[i].c] =
        (rgb16_t){0x0FFF, 0, 0};  // červená barva
  }
}

// Demo obsah: 12bit barvy (0..4095) – zároveň uložíme baseline do fb0
static void fb_init_content(void) {
  for (int c = 0; c < COLS; ++c) {
    for (int r = 0; r < ROWS; ++r) {
      //   if (r == 0 || r == ROWS - 1 || c == 0 || c == COLS - 1) {
      //     fb[r][c] = (rgb16_t){0x005f, 0x005f, 0x005f};
      //   } else {
      fb[r][c] = (rgb16_t){0, 0, 0};
      //   }
    }
  }
  // založ baseline kopii
  memcpy(fb0, fb, sizeof(fb0));
}

// Sníží jas celé matice na určité procento (0-100)
void set_brightness(uint8_t percent) {
  if (percent > 100) percent = 100;

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      fb[r][c].r = (fb0[r][c].r * percent) / 100;
      fb[r][c].g = (fb0[r][c].g * percent) / 100;
      fb[r][c].b = (fb0[r][c].b * percent) / 100;
    }
  }
}

void game_init() { queue_push(&direction, DIR_RIGHT); }

void app_main(void) {
  // GPIO 74HCT154
  gpio_config_t io = {
      .pin_bit_mask = (1ULL << HCT154_ADDR0) | (1ULL << HCT154_ADDR1) |
                      (1ULL << HCT154_ADDR2) | (1ULL << HCT154_ADDR3) |
                      (1ULL << HCT154_COL_EN),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = 0,
      .pull_down_en = 0,
      .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io);
  col_disable_all();
  col_select(0);

  // Tlačítko na GPIO 22
  gpio_config_t btn_cfg = {
      .pin_bit_mask = (1ULL << HTC154_SW1 | 1ULL << HTC154_SW2 |
                       1ULL << HTC154_SW3 | 1ULL << HTC154_SW4),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en =
          GPIO_PULLUP_ENABLE,  // Pull-up zapnutý (tlačítko spíná na zem)
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE  // Interrupt na falling edge (HIGH→LOW)
  };
  gpio_config(&btn_cfg);

  // Instalace ISR služby a přidání handleru
  gpio_install_isr_service(0);
  gpio_isr_handler_add(HTC154_SW1, button_isr_handler, (void *)DIR_DOWN);
  gpio_isr_handler_add(HTC154_SW2, button_isr_handler, (void *)DIR_UP);
  gpio_isr_handler_add(HTC154_SW3, button_isr_handler, (void *)DIR_LEFT);
  gpio_isr_handler_add(HTC154_SW4, button_isr_handler, (void *)DIR_RIGHT);

  // TLC5947
  tlc5947_config_t cfg = {.host = SPI3_HOST,
                          .mosi_io = TLC_MOSI,
                          .sclk_io = TLC_SCLK,
                          .xlat_io = TLC_XLAT,
                          .blank_io = TLC_BLANK,
                          .chips = 1,
                          .clock_hz = 30 * 1000 * 1000,
                          .dma_chan = 0};
  ESP_ERROR_CHECK(tlc5947_init(&tlc, &cfg));
  tlc5947_fill(&tlc, 0);
  tlc5947_update(&tlc, true);

  fb_init_content();
  set_brightness(50);  // 50% jasu
  game_init();

  // Timery
  const esp_timer_create_args_t scan_tmr_args = {.callback = &scan_timer_cb,
                                                 .name = "scan"};
  const esp_timer_create_args_t frame_tmr_args = {.callback = &frame_timer_cb,
                                                  .name = "frame"};
  esp_timer_handle_t scan_tmr, frame_tmr;
  ESP_ERROR_CHECK(esp_timer_create(&scan_tmr_args, &scan_tmr));
  ESP_ERROR_CHECK(esp_timer_create(&frame_tmr_args, &frame_tmr));
  ESP_ERROR_CHECK(esp_timer_start_periodic(scan_tmr, COL_DWELL_US));
  ESP_ERROR_CHECK(esp_timer_start_periodic(frame_tmr, 1000000 / GAME_RATE_HZ));

  while (1) {
    // Zde MŮŽEŠ volat printf() - není to ISR
    if (button_pressed) {
      button_pressed = false;  // Reset flagu
      printf("Tlačítko stisknuto!\n");
      for (int i = 0; i < 5; i++) {
        Direction dir = DIR_EMPTY;
        queue_peek_last(&direction, &dir);
        printf("Směr v queue: %d\n", dir);
      }
      printf("Test direction: %d\n", test);
      // Zde můžeš změnit směr hada nebo cokoliv jiného
      // Například: snake_dir = (snake_dir + 1) % 4;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
