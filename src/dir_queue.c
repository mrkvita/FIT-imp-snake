#include "dir_queue.h"

#include "models.h"

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