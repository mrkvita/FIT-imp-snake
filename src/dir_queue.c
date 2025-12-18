/**
 * @file dir_queue.c
 * @brief Implementation of queue of directions for snake movement.
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024
 */
#include "dir_queue.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "models.h"

// Static mutex for critical section protection -> wirte happens on interrup and
// that could corrupt queue size
static portMUX_TYPE queue_mux = portMUX_INITIALIZER_UNLOCKED;

void queue_push(Queue *queue, Direction dir) {
  taskENTER_CRITICAL(&queue_mux);
  if (queue->occupied >= QUEUE_SIZE) {
    taskEXIT_CRITICAL(&queue_mux);
    return;  // Queue is full, do not add new direction
  }
  queue->q[queue->tail] = dir;
  queue->tail = (queue->tail + 1) % QUEUE_SIZE;
  queue->occupied++;
  taskEXIT_CRITICAL(&queue_mux);
}

void queue_pop(Queue *queue, Direction *dir) {
  taskENTER_CRITICAL(&queue_mux);
  if (queue->occupied == 0) {
    taskEXIT_CRITICAL(&queue_mux);
    return;
  }
  *dir = queue->q[queue->head];
  queue->head = (queue->head + 1) % QUEUE_SIZE;
  queue->occupied--;
  taskEXIT_CRITICAL(&queue_mux);
}

void queue_peek(Queue *queue, Direction *dir) {
  taskENTER_CRITICAL(&queue_mux);
  if (queue->occupied == 0) {
    taskEXIT_CRITICAL(&queue_mux);
    return;
  }
  *dir = queue->q[queue->head];
  taskEXIT_CRITICAL(&queue_mux);
}

void queue_peek_last(Queue *queue, Direction *dir) {
  taskENTER_CRITICAL(&queue_mux);
  if (queue->occupied == 0) {
    taskEXIT_CRITICAL(&queue_mux);
    return;
  }
  size_t last_index = (queue->tail + QUEUE_SIZE - 1) % QUEUE_SIZE;
  *dir = queue->q[last_index];
  taskEXIT_CRITICAL(&queue_mux);
}

/******************************EOFdir_queue.c*******************************/