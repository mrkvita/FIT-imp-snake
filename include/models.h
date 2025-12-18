/**
 * @file models.h
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024
 */
#ifndef MY_MODELS_H
#define MY_MODELS_H

#include <stdint.h>

#include "tlc5947.h"

#define QUEUE_SIZE 5

// Pixel
typedef struct {
  uint16_t r, g, b;
} rgb16_t;

typedef enum {
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT,
  DIR_EMPTY,
} Direction;

// Position in the framebuffer
typedef struct {
  int r;
  int c;
} Pos;

// Direction with metadata
typedef struct {
  Pos pos;
  Direction name;
  Direction opposite;
} Dir;

// Snake model
typedef struct {
  Pos body[ROWS * COLS];  // depends on the display
  size_t len;
  volatile Dir dir;
} Snake;

// Queue of directions
typedef struct {
  Direction q[QUEUE_SIZE];  // buffer 5 last directions
  size_t head;
  size_t tail;
  size_t occupied;
} Queue;

typedef enum { DIFF_EASY, DIFF_MEDIUM, DIFF_HARD } Difficulty;

// Difficulty settings
typedef struct {
  Difficulty name;
  uint16_t move_T;       // the period for snake movement in game ticks
  uint16_t food_T;       // the period for fruit spawning in game ticks
  uint16_t evil_food_T;  // the period for evil fruit spawning in game ticks
  uint8_t food_spawn_chance;  // the chance (in %) of spawning a fruit
  uint8_t
      evil_food_spawn_chance;  // the chance (in %) of spawning an evil fruit
  uint8_t max_fruit;           // maximum number of fruits on the field at once
  uint8_t max_evil_fruit;  // maximum number of evil fruits on the field at once
  uint16_t fruit_ttl;      // time to live of a fruit in game ticks
  uint16_t evil_fruit_ttl;  // time to live of an evil fruit in game ticks
  size_t winning_len;       // length of the snake needed to win
  size_t min_snake_len;     // minimum length of the snake, WARNING: must be <=
                            // COLS
  uint8_t good_inc;         // how much the snake grows when eating a fruit
  uint8_t evil_dec;  // how much the snake shrinks when eating an evil fruit
} Dif;

// Fruit model
typedef struct {
  Pos pos;
  bool is_evil;
  bool enabled;
  uint16_t ttl;
} Fruit;

typedef enum { GAME_RUNNING, GAME_IDLE, GAME_WON, GAME_LOST } State;

// Game model
typedef struct {
  Snake snake;
  volatile State state;
  Dif difficulty;
  Fruit fruits[ROWS * COLS];
  size_t fruit_count;
  size_t evil_fruit_count;
  int buffered_len;
} GameManager;

// Constants (defined in models.c)
extern const Dir DIR_DELTA[4];
extern const Dif DIFFICULTIES[3];
extern const rgb16_t SNAKE_COLOR;
extern const rgb16_t SNAKE_HEAD_COLOR;
extern const rgb16_t FRUIT_COLOR;
extern const rgb16_t EVIL_FRUIT_COLOR;
extern const rgb16_t TEXT_COLOR;
extern const rgb16_t EASY_COLOR;
extern const rgb16_t MEDIUM_COLOR;
extern const rgb16_t HARD_COLOR;
extern const rgb16_t SELECTED_COLOR;
extern const rgb16_t LOST_COLOR;
extern const rgb16_t WON_COLOR;
extern const size_t MAX_GAME_ARRAY_LEN;
extern const size_t MIN_GAME_ARRAY_LEN;
#endif