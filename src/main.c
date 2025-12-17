#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir_queue.h"
#include "draw.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "game.h"
#include "globals.h"
#include "models.h"
#include "tlc5947.h"
#include "utils.h"

// ==== SIGNALS AND GLOBALS ====
Queue direction = {.q = {}, .head = 0, .tail = 0};
GameManager gm;
static volatile bool game_start_requested = false;
static volatile bool idle_requested = false;
static volatile bool next_difficulty_requested = false;
static volatile bool prev_difficulty_requested = false;
static volatile bool game_restart_requested = false;
volatile bool fb_swap_pending = false;  // used by draw and scan to coordinate
                                        // frame swapping at frame boundary;
/************************** DISCLAIMER ******************************
 * The following code is taken from the example project attached to *
 * the assignment.                                                  *
 * Parts were removed or modified to fit the needs of this project. *
 * The copied section ends with asterisk styled comment.            *
 ********************************************************************/

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
// Buttons
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
#define FRAME_RATE_HZ 50
#define GAME_RATE_HZ \
  20  // the game logic updates at 20 Hz --- CAREFUL the game difficulty is tied
      // to this
#define COL_DWELL_US (1000000 / (FRAME_RATE_HZ * COLS))
// --- 16bit framebuffer (hodnoty 0..4095) ---
rgb16_t fb_buf0[ROWS][COLS];            // storage buffer 0
rgb16_t fb_buf1[ROWS][COLS];            // storage buffer 1
rgb16_t (*fb_draw)[COLS] = fb_buf0;     // For drawing
rgb16_t (*fb_display)[COLS] = fb_buf1;  // For displaying
rgb16_t fb0[ROWS][COLS];                // baseline kopie pro obnovení
// Ovladač TLC
static tlc5947_t tlc;
// Stav multiplexu
static volatile int cur_col = -1;

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
    rgb16_t px = fb_display[r][col];
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

  // vblank --- only swap the buffers at the frame boundary --- ensures the
  // buffer is consistent throughout the frame
  if (cur_col == 0 && fb_swap_pending) {
    rgb16_t(*tmp)[COLS] = fb_display;
    fb_display = fb_draw;
    fb_draw = tmp;
    fb_swap_pending = false;
  }

  bool lit = true;
  load_column_into_tlc(cur_col, lit);
  tlc5947_update(&tlc, true);  // BLANK-sync latch

  col_select(cur_col);
  col_enable_selected();
}

static void fb_init_content(void) {
  for (int c = 0; c < COLS; ++c) {
    for (int r = 0; r < ROWS; ++r) {
      fb_draw[r][c] = (rgb16_t){0, 0, 0};
    }
  }
  // založ baseline kopii
  memcpy(fb0, fb_draw, sizeof(fb0));
  memcpy(fb_display, fb_draw, sizeof(fb_buf0));
}

/**********************END OF THE COPIED SECTION********************/

// ==== HANDLING BUTTON PRESSES, BUTTON BINDINGS =====
static void binds_idle(Direction dir) {
  switch (dir) {
    case DIR_UP:
      game_start_requested = true;
      break;
    case DIR_DOWN:
      game_start_requested = true;
      break;
    case DIR_LEFT:
      prev_difficulty_requested = true;
      break;
    case DIR_RIGHT:
      next_difficulty_requested = true;
      break;
    case DIR_EMPTY:
      break;
  }
}
static void binds_end_game(Direction dir) {
  static int count_d = 0;
  static int count_u = 0;
  switch (dir) {
    case DIR_UP:
      count_u++;
      if (count_u >= 2) {
        count_u = 0;
        count_d = 0;
        game_restart_requested = true;
      }
      break;
    case DIR_DOWN:
      count_d++;
      if (count_d >= 2) {
        count_u = 0;
        count_d = 0;
        idle_requested = true;
      }
      break;
    default:
      break;
  }
}
static void binds_running(Direction dir) {
  // during game, all buttons insert direction
  insert_dir(dir, &gm);
}

// ==== INTERRUPT HANDLER FOR BUTTONS =====
static void IRAM_ATTR button_isr_handler(void *arg) {
  /* Handle debouncing and spamming - the time limit depends on game state to
   * make the game responsive */
  int max_time = 0;
  if (gm.state == GAME_IDLE) max_time = 250;
  if (gm.state == GAME_LOST || gm.state == GAME_WON) max_time = 100;
  static int64_t last_press = 0;
  int64_t now = esp_timer_get_time() / 1000;  // in ms
  if (now - last_press < max_time) {
    return;
  }
  last_press = now;

  // Call appropriate binding based on game state
  Direction dir = (Direction)(uintptr_t)arg;
  switch (gm.state) {
    case GAME_IDLE:
      binds_idle(dir);
      break;
    case GAME_RUNNING:
      binds_running(dir);
      break;
    case GAME_LOST:
    case GAME_WON:
      binds_end_game(dir);
      break;
    default:
      break;
  }
}

// ===== GAME STATE FUNCTIONS =====

void game_init(Difficulty diff) {
  Direction start_dir = DIR_RIGHT;
  gm.snake.body[0] = (Pos){ROWS / 2, COLS / 2 + 5};
  gm.snake.body[1] = (Pos){ROWS / 2, COLS / 2 + 4};
  gm.snake.body[2] = (Pos){ROWS / 2, COLS / 2 + 3};
  gm.snake.body[3] = (Pos){ROWS / 2, COLS / 2 + 2};
  gm.snake.body[4] = (Pos){ROWS / 2, COLS / 2 + 1};
  gm.snake.body[5] = (Pos){ROWS / 2, COLS / 2};
  gm.snake.len = 6;
  gm.snake.dir = DIR_DELTA[start_dir];
  gm.state = GAME_IDLE;
  gm.difficulty = DIFFICULTIES[diff];
  memset(gm.fruits, 0, sizeof(gm.fruits));
  gm.fruit_count = 0;
  gm.evil_fruit_count = 0;
  gm.buffered_len = 0;
  queue_push(&direction, start_dir);
}

void game_won() {
  draw_won();
  if (idle_requested) {
    idle_requested = false;
    game_init(DIFF_EASY);
    gm.state = GAME_IDLE;
  }
  if (game_restart_requested) {
    game_restart_requested = false;
    game_init(gm.difficulty.name);
    gm.state = GAME_RUNNING;
  }
}

void game_lost() {
  draw_lost();
  if (idle_requested) {
    idle_requested = false;
    game_init(DIFF_EASY);
    gm.state = GAME_IDLE;
  }
  if (game_restart_requested) {
    game_restart_requested = false;
    game_init(gm.difficulty.name);
    gm.state = GAME_RUNNING;
  }
}

void game_idle() {
  draw_idle(&gm);
  if (next_difficulty_requested) {
    gm.difficulty = DIFFICULTIES[get_next_difficulty(gm.difficulty.name)];
    next_difficulty_requested = false;
  }
  if (prev_difficulty_requested) {
    gm.difficulty = DIFFICULTIES[get_prev_difficulty(gm.difficulty.name)];
    prev_difficulty_requested = false;
  }
  if (game_start_requested) {
    game_start_requested = false;
    fb_clear();
    fb_swap();  // clear display before starting
    gm.state = GAME_RUNNING;
  }
}

void game_running() {
  draw_running(&gm);
  static int frame_counter = 0;
  frame_counter++;
  if (frame_counter >= gm.difficulty.move_T) {
    frame_counter = 0;
    move_snake(&gm, &direction);
    State res = check_conditions(&gm);
    if (res != GAME_RUNNING) {
      gm.state = res;
      return;
    }
  }
  spawn_fruit(&gm);
  remove_expired_fruits(&gm);
}

// Game režie – běží 50 Hz
static void IRAM_ATTR game_timer_cb(void *arg) {
  switch (gm.state) {
    case GAME_IDLE:
      game_idle();
      break;
    case GAME_RUNNING:
      game_running();
      break;
    case GAME_WON:
      game_won();
      break;
    case GAME_LOST:
      game_lost();
      break;
    default:
      break;
  }
}

/************************** DISCLAIMER *******************************
 * Parts of the app_main function are taken form the example project *
 * attached to the assignment                                     .  *
 * Parts were removed or modified to fit the needs of this project.  *
 *********************************************************************/
void app_main(void) {
  game_init(DIFF_EASY);
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

  // Buttons
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

  // ISR for buttons
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

  // Timer for display multiplexing
  const esp_timer_create_args_t scan_tmr_args = {.callback = &scan_timer_cb,
                                                 .name = "scan"};
  const esp_timer_create_args_t frame_tmr_args = {.callback = &game_timer_cb,
                                                  .name = "game"};
  esp_timer_handle_t scan_tmr, frame_tmr;
  ESP_ERROR_CHECK(esp_timer_create(&scan_tmr_args, &scan_tmr));
  ESP_ERROR_CHECK(esp_timer_create(&frame_tmr_args, &frame_tmr));
  ESP_ERROR_CHECK(esp_timer_start_periodic(scan_tmr, COL_DWELL_US));
  ESP_ERROR_CHECK(esp_timer_start_periodic(frame_tmr, 1000000 / GAME_RATE_HZ));

  // Main game loop
  while (1) vTaskDelay(pdMS_TO_TICKS(100));
}
