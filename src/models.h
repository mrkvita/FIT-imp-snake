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
  Pos body[ROWS * COLS];
  size_t len;
  Dir dir;
} Snake;

typedef struct {
  Direction q[QUEUE_SIZE];  // buffer 5 last directions
  size_t head;
  size_t tail;
  size_t occupied;
} Queue;

#endif