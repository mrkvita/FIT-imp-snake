/**
 * @file globals.h
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#include "freertos/FreeRTOS.h"
#include "models.h"

// Globals (defined in main.c)
// Direction input queue
extern Queue direction;
// Double-buffered framebuffers.
extern rgb16_t (*fb_display)[COLS];  // scanned
extern rgb16_t (*fb_draw)[COLS];     // rendered to
extern rgb16_t fb_buf0[ROWS][COLS];
extern rgb16_t fb_buf1[ROWS][COLS];
extern rgb16_t fb0[ROWS][COLS];  // baseline copy of initial cleared buffer

// Frame is ready to be swapped and displayed
extern volatile bool fb_swap_pending;

#endif
