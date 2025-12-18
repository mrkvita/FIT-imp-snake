/**
 * @file draw.c
 * @brief Implementation of drawing functions for different game states, framebuffer operations
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024 
 */
#include "draw.h"
#include "globals.h"
#include "models.h"
#include <string.h>

/**
 * @brief Replaces the current framebuffer with the baseline copy.  
 * WARNING: Depends on global fb0 and fb_draw.
 */
void fb_clear() {
  // Clear the current draw buffer to the baseline.
  memcpy(fb_draw, fb0, sizeof(fb_buf0));
}

/**
 * @brief Requests a framebuffer swap at the next frame boundary.
 * WARNING: Depends on global fb_swap_pending.
 */
void fb_swap() {
  // Request to publish the frame --- the actual swap happens at frame boundary. 
  fb_swap_pending = true;
}

// ==== DRAWING GAME STATES =====

/**
 * @brief Draws the wining screen to the framebuffer.
 * WARNING: Depends on global fb_draw and colors defined in models.h.
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

/**
 * @brief Draws the losing screen to the framebuffer.
 * WARNING: Depends on global fb_draw and colors defined in models.h.
 */
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

/**
 * @brief Draws the idle screen to the framebuffer, including current difficulty.
 * WARNING: Depends on global fb_draw, colors defined in models.h
 */
void draw_idle(GameManager *gm) {
  if (gm == NULL) return;
  fb_clear();
  // H
  int padding_top = 2;
  int current = 1;
  int current_dif = 3;
  int space = 3;
  fb_draw[ROWS - padding_top - 0][current] = TEXT_COLOR; // left vertical
  fb_draw[ROWS - padding_top - 1][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current + 1] = TEXT_COLOR; // middle horizontal
  fb_draw[ROWS - padding_top - 2][current + 1] = TEXT_COLOR; 
  fb_draw[ROWS - padding_top - 0][current + 2] = TEXT_COLOR; // right vertical
  fb_draw[ROWS - padding_top - 1][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current + 2] = TEXT_COLOR;
  current += 2 + space;
  // A
  fb_draw[ROWS - padding_top - 1][current] = TEXT_COLOR; // left  vertical
  fb_draw[ROWS - padding_top - 2][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 0][current + 1] = TEXT_COLOR;  // top horizontal
  fb_draw[ROWS - padding_top - 0][current + 2] = TEXT_COLOR;  
  fb_draw[ROWS - padding_top - 2][current + 1] = TEXT_COLOR;  // middle horizontal
  fb_draw[ROWS - padding_top - 2][current + 2] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 1][current + 3] = TEXT_COLOR;  // right vertical
  fb_draw[ROWS - padding_top - 2][current + 3] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current + 3] = TEXT_COLOR;
  current += 3 + space;
  // D
  fb_draw[ROWS - padding_top - 0][current] = TEXT_COLOR; // left vertical
  fb_draw[ROWS - padding_top - 1][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 2][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 3][current] = TEXT_COLOR;
  fb_draw[ROWS - padding_top - 0][current + 1] = TEXT_COLOR; // top horizontal
  fb_draw[ROWS - padding_top - 3][current + 1] = TEXT_COLOR; // bottom horizontal
  fb_draw[ROWS - padding_top - 1][current + 2] = TEXT_COLOR; // right vertical
  fb_draw[ROWS - padding_top - 2][current + 2] = TEXT_COLOR;
  // Difficulties
  fb_draw[ROWS - 5 - padding_top][current_dif] = EASY_COLOR;
  fb_draw[ROWS - 5 - padding_top][current_dif + 1] = EASY_COLOR;
  if (gm->difficulty.name == DIFF_EASY) {
    fb_draw[ROWS - 5 - padding_top][current_dif - 1] = SELECTED_COLOR;
    fb_draw[ROWS - 5 - padding_top][current_dif + 2] = SELECTED_COLOR;
  }
  current_dif += space + 1;
  fb_draw[ROWS - 5 - padding_top][current_dif] = MEDIUM_COLOR;
  fb_draw[ROWS - 5 - padding_top][current_dif + 1] = MEDIUM_COLOR;
  if (gm->difficulty.name == DIFF_MEDIUM) {
    fb_draw[ROWS - 5 - padding_top][current_dif - 1] = SELECTED_COLOR;
    fb_draw[ROWS - 5 - padding_top][current_dif + 2] = SELECTED_COLOR;
  }
  current_dif += space + 1;
  fb_draw[ROWS - 5 - padding_top][current_dif] = HARD_COLOR;
  fb_draw[ROWS - 5 - padding_top][current_dif + 1] = HARD_COLOR;
  if (gm->difficulty.name == DIFF_HARD) {
    fb_draw[ROWS - 5 - padding_top][current_dif - 1] = SELECTED_COLOR;
    fb_draw[ROWS - 5 - padding_top][current_dif + 2] = SELECTED_COLOR;
  }
  fb_swap();
}

/**
 * @brief Draws the running game state to the framebuffer, including snake and fruits.
 * WARNING: Depends on global fb_draw and colors defined in models.h.
 */
void draw_running(GameManager *gm) {
  if (gm == NULL) return;
  fb_clear();
  // draw snake
  for (size_t i = 0; i < gm->snake.len; i++) {
    fb_draw[gm->snake.body[i].r][gm->snake.body[i].c] = SNAKE_COLOR;
  }
  fb_draw[gm->snake.body[0].r][gm->snake.body[0].c] = SNAKE_HEAD_COLOR;
  // draw fruits
  for (size_t i = 0; i < MAX_GAME_ARRAY_LEN; i++) {
    if (!gm->fruits[i].enabled) continue;
    rgb16_t color = gm->fruits[i].is_evil ? EVIL_FRUIT_COLOR 
                                         :  FRUIT_COLOR;
    fb_draw[gm->fruits[i].pos.r][gm->fruits[i].pos.c] = color;
  }
  fb_swap();
};

/**************************************EOF draw.c*************************************/