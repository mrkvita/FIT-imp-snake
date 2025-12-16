#ifndef MY_MODELS_H
#define MY_MODELS_H

#include "tlc5947.h"

#define QUEUE_SIZE 5

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
  unsigned speed;
  unsigned food_freq;
  unsigned max_fruit;
  unsigned max_evil_fruit;
  size_t min_snake_len;
  size_t good_inc;  // how much the snake grows when eating a fruit
  size_t evil_dec;  // how much the snake shrinks when eating an evil fruit
} Dif;

// Declared extern (defined in models.c)
extern const Dir DIR_DELTA[4];
extern const Dif DIFFICULTIES[3];

typedef struct {
  Pos pos;
  bool is_evil;
} Fruit;

typedef enum { GAME_RUNNING, GAME_IDLE } State;

typedef struct {
  Snake snake;
  volatile State state;
  Dif difficulty;
  Fruit fruits[ROWS * COLS];
  size_t fruit_count;
} GameManager;

#endif