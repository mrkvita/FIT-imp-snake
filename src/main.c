#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir_queue.h"
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
#define FRAME_RATE_HZ 60
#define GAME_RATE_HZ \
  20  // the game logic updates at 20 Hz --- CAREFUL the game difficulty is tied
      // to this
#define COL_DWELL_US (1000000 / (FRAME_RATE_HZ * COLS))
// --- 16bit framebuffer (hodnoty 0..4095) ---
static rgb16_t fb_draw[ROWS][COLS];     // For drawing 
static rgb16_t fb_display[ROWS][COLS];  // For displaying 
static rgb16_t fb0[ROWS][COLS];         // baseline kopie pro obnovení
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
    rgb16_t px = fb_display[r][col];  // ISR reads from display buffer
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

static void fb_init_content(void) {
  for (int c = 0; c < COLS; ++c) {
    for (int r = 0; r < ROWS; ++r) {
      fb_draw[r][c] = (rgb16_t){0, 0, 0};
    }
  }
  // založ baseline kopii
  memcpy(fb0, fb_draw, sizeof(fb0));
  memcpy(fb_display, fb_draw, sizeof(fb_display));
}

/**********************END OF THE COPIED SECTION ********************/

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
  insert_dir(dir);
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

// ==== FUNCTIONS FOR MANAGING FRAMEBUFFERS =====
static void fb_clear() { memcpy(fb_draw, fb0, sizeof(fb_draw)); }
static void fb_swap() {
  // copy draw buffer to display buffer
  memcpy(fb_display, fb_draw, sizeof(fb_display));  // isr sees the new frame
}

// ==== DRAWING GAME STATES =====
/* WARNING: These functions assume one fixed size of the display
 *          matrix.
 */
void draw_won() {
  fb_clear();
  // Draw W
  fb_draw[ROWS - 1][2] = WON_COLOR;
  fb_draw[ROWS - 2][2] = WON_COLOR;
  fb_draw[ROWS - 1][13] = WON_COLOR;
  fb_draw[ROWS - 2][13] = WON_COLOR;
  fb_draw[ROWS - 3][3] = WON_COLOR;
  fb_draw[ROWS - 4][3] = WON_COLOR;
  fb_draw[ROWS - 3][7] = WON_COLOR;
  fb_draw[ROWS - 4][7] = WON_COLOR;
  fb_draw[ROWS - 3][8] = WON_COLOR;
  fb_draw[ROWS - 4][8] = WON_COLOR;
  fb_draw[ROWS - 3][12] = WON_COLOR;
  fb_draw[ROWS - 4][12] = WON_COLOR;
  fb_draw[ROWS - 5][4] = WON_COLOR;
  fb_draw[ROWS - 6][4] = WON_COLOR;
  fb_draw[ROWS - 5][6] = WON_COLOR;
  fb_draw[ROWS - 6][6] = WON_COLOR;
  fb_draw[ROWS - 5][9] = WON_COLOR;
  fb_draw[ROWS - 6][9] = WON_COLOR;
  fb_draw[ROWS - 5][11] = WON_COLOR;
  fb_draw[ROWS - 6][11] = WON_COLOR;
  fb_draw[ROWS - 7][5] = WON_COLOR;
  fb_draw[ROWS - 8][5] = WON_COLOR;
  fb_draw[ROWS - 7][10] = WON_COLOR;
  fb_draw[ROWS - 8][10] = WON_COLOR;
  fb_swap();
}

void draw_lost() {
  fb_clear();
  // Letter L
  fb_draw[ROWS - 1][5] = LOST_COLOR;
  fb_draw[ROWS - 2][5] = LOST_COLOR;
  fb_draw[ROWS - 3][5] = LOST_COLOR;
  fb_draw[ROWS - 4][5] = LOST_COLOR;
  fb_draw[ROWS - 5][5] = LOST_COLOR;
  fb_draw[ROWS - 6][5] = LOST_COLOR;
  fb_draw[ROWS - 7][5] = LOST_COLOR;
  fb_draw[ROWS - 8][5] = LOST_COLOR;
  fb_draw[ROWS - 1][6] = LOST_COLOR;
  fb_draw[ROWS - 2][6] = LOST_COLOR;
  fb_draw[ROWS - 3][6] = LOST_COLOR;
  fb_draw[ROWS - 4][6] = LOST_COLOR;
  fb_draw[ROWS - 5][6] = LOST_COLOR;
  fb_draw[ROWS - 6][6] = LOST_COLOR;
  fb_draw[ROWS - 7][6] = LOST_COLOR;
  fb_draw[ROWS - 8][6] = LOST_COLOR;
  fb_draw[ROWS - 8][7] = LOST_COLOR;
  fb_draw[ROWS - 7][7] = LOST_COLOR;
  fb_draw[ROWS - 8][8] = LOST_COLOR;
  fb_draw[ROWS - 7][8] = LOST_COLOR;
  fb_draw[ROWS - 8][9] = LOST_COLOR;
  fb_draw[ROWS - 7][9] = LOST_COLOR;
  fb_draw[ROWS - 8][10] = LOST_COLOR;
  fb_draw[ROWS - 7][10] = LOST_COLOR;
  fb_swap();
}

void draw_idle() {
  fb_clear();
  // H
  int padding_top = 2;
  int current = 1;
  int current_dif = 3;
  int space = 3;
  fb_draw[ROWS - padding_top - 0][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current + 1] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 0][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current + 2] = TEXT_COLOR;
  current += 2 + space;
  // A
  fb_draw[ROWS - padding_top - 1][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 0][current + 1] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current + 1] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 0][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current + 3] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current + 3] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current + 3] = TEXT_COLOR;
  current += 3 + space;
  // D
  fb_draw[ROWS - padding_top - 0][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 0][current + 1] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current + 1] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current + 2] = TEXT_COLOR;
  // Difficulties
  fb_draw[ROWS - 5 - padding_top][current_dif] = EASY_COLOR;
  fb_draw[ROWS - 5 - padding_top][current_dif + 1] = EASY_COLOR;
  if (gm.difficulty.name == DIFF_EASY) {
    fb_draw[ROWS - 5 - padding_top][current_dif - 1] = SELECTED_COLOR;
    fb_draw[ROWS - 5 - padding_top][current_dif + 2] = SELECTED_COLOR;
  }
  current_dif += space + 1;
  fb_draw[ROWS - 5 - padding_top][current_dif] = MEDIUM_COLOR;
  fb_draw[ROWS - 5 - padding_top][current_dif + 1] = MEDIUM_COLOR;
  if (gm.difficulty.name == DIFF_MEDIUM) {
    fb_draw[ROWS - 5 - padding_top][current_dif - 1] = SELECTED_COLOR;
    fb_draw[ROWS - 5 - padding_top][current_dif + 2] = SELECTED_COLOR;
  }
  current_dif += space + 1;
  fb_draw[ROWS - 5 - padding_top][current_dif] = HARD_COLOR;
  fb_draw[ROWS - 5 - padding_top][current_dif + 1] = HARD_COLOR;
  if (gm.difficulty.name == DIFF_HARD) {
    fb_draw[ROWS - 5 - padding_top][current_dif - 1] = SELECTED_COLOR;
    fb_draw[ROWS - 5 - padding_top][current_dif + 2] = SELECTED_COLOR;
  }
  fb_swap();
}

void draw_running() {
  fb_clear();
  // draw snake
  for (size_t i = 0; i < gm.snake.len; i++) {
    fb_draw[gm.snake.body[i].r][gm.snake.body[i].c] = SNAKE_COLOR;
  }
  fb_draw[gm.snake.body[0].r][gm.snake.body[0].c] = SNAKE_HEAD_COLOR;
  // draw fruits
  for (size_t i = 0; i < MAX_GAME_ARRAY_LEN; i++) {
    if (!gm.fruits[i].enabled) continue;
    rgb16_t color = gm.fruits[i].is_evil ? (rgb16_t){0x0FFF, 0, 0}   // red
                                         : (rgb16_t){0, 0x0FFF, 0};  // green
    fb_draw[gm.fruits[i].pos.r][gm.fruits[i].pos.c] = color;
  }
  fb_swap();
};

// ===== GAME STATE FUNCTIONS =====
void move_snake() {
  Direction next_dir = gm.snake.dir.name;
  queue_pop(&direction, &next_dir);  // invariant if empty
  gm.snake.dir = DIR_DELTA[next_dir];

  // Increment snake from length buffer
  if (gm.buffered_len > 0) {
    if (gm.snake.len < MAX_GAME_ARRAY_LEN) {
      gm.snake.len++;  // means it will get redrawn at the end
    }
    gm.buffered_len--;
  }
  if (gm.buffered_len < 0) {
    if (gm.snake.len > MIN_GAME_ARRAY_LEN) {
      gm.snake.len--;
    }
    gm.buffered_len++;
  }

  for (int i = gm.snake.len - 1; i > 0; i--) {
    gm.snake.body[i] = gm.snake.body[i - 1];
  }
  // new head
  gm.snake.body[0].r += gm.snake.dir.pos.r;
  gm.snake.body[0].c += gm.snake.dir.pos.c;
  gm.snake.body[0].r = (gm.snake.body[0].r + ROWS) % ROWS;
  gm.snake.body[0].c = (gm.snake.body[0].c + COLS) % COLS;
}

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
  draw_idle();
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
  draw_running();
  static int frame_counter = 0;
  frame_counter++;
  if (frame_counter >= gm.difficulty.move_T) {
    frame_counter = 0;
    move_snake();
    State res = check_conditions(&gm);
    if (res != GAME_RUNNING) {
      gm.state = res;
      return;
    }
  }
  spawn_fruit(&gm);
  remove_expired_fruits(&gm);
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
  esp_timer_handle_t scan_tmr;
  ESP_ERROR_CHECK(esp_timer_create(&scan_tmr_args, &scan_tmr));
  ESP_ERROR_CHECK(esp_timer_start_periodic(scan_tmr, COL_DWELL_US));

  // Main game loop
  while (1) {
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
    vTaskDelay(pdMS_TO_TICKS(1000 / (GAME_RATE_HZ))); 
  }
}
