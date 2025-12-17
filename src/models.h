#ifndef MY_MODELS_H
#define MY_MODELS_H

#include <stdint.h>

#include "tlc5947.h"

#define QUEUE_SIZE 5

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
  Pos body[ROWS * COLS];  // depends on the display
  size_t len;
  Dir dir;
} Snake;

typedef struct {
  Direction q[QUEUE_SIZE];  // buffer 5 last directions
  size_t head;
  size_t tail;
  size_t occupied;
} Queue;

typedef enum { DIFF_EASY, DIFF_MEDIUM, DIFF_HARD } Difficulty;

typedef struct {
  Difficulty name;
  uint16_t speed;
  uint16_t food_T;
  uint8_t max_fruit;
  uint8_t max_evil_fruit;
  uint16_t fruit_ttl;
  uint16_t evil_fruit_ttl;
  size_t winning_len;
  size_t min_snake_len;
  uint8_t good_inc;  // how much the snake grows when eating a fruit
  uint8_t evil_dec;  // how much the snake shrinks when eating an evil fruit
} Dif;

typedef struct {
  Pos pos;
  bool is_evil;
  bool enabled;
  uint8_t ttl;
} Fruit;

typedef enum { GAME_RUNNING, GAME_IDLE, GAME_WON, GAME_LOST } State;

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
#endif