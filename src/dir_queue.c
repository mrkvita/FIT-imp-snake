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

/**
 * @brief Pushes a new direction into the queue.
 * @param queue Pointer to the Queue structure.
 * @param dir The direction to push.
 * @note If the queue is full, the direction is not added.
 */
void queue_push(Queue *queue, Direction dir) {
  // https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32c3/api-guides/freertos-smp.html#critical-sections-disabling-interrupts
  if (queue == NULL) return;
  taskENTER_CRITICAL(&queue_mux);  // can be entered from isr as well it points
                                   // to the same function
  if (queue->occupied >= QUEUE_SIZE) {
    taskEXIT_CRITICAL(&queue_mux);
    return;  // Queue is full, do not add new direction
  }
  queue->q[queue->tail] = dir;
  queue->tail = (queue->tail + 1) % QUEUE_SIZE;
  queue->occupied++;
  taskEXIT_CRITICAL(&queue_mux);
}

/**
 * @brief Pops a direction from the queue.
 * @param queue Pointer to the Queue structure.
 * @param dir Pointer to store the popped direction.
 * @note If the queue is empty, no direction is popped.
 * @note If any param is null nothing happens.
 */
void queue_pop(Queue *queue, Direction *dir) {
  if (queue == NULL || dir == NULL) return;
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

/**
 * @brief Peeks at the front direction of the queue without removing it.
 * @param queue Pointer to the Queue structure.
 * @param dir Pointer to store the peeked direction.
 * @note If the queue is empty, no direction is returned.
 * @note If any param is null nothing happens.
 */
void queue_peek(Queue *queue, Direction *dir) {
  if (queue == NULL || dir == NULL) return;
  taskENTER_CRITICAL(&queue_mux);
  if (queue->occupied == 0) {
    taskEXIT_CRITICAL(&queue_mux);
    return;
  }
  *dir = queue->q[queue->head];
  taskEXIT_CRITICAL(&queue_mux);
}

/**
 * @brief Peeks at the last direction of the queue without removing it.
 * @param queue Pointer to the Queue structure.
 * @param dir Pointer to store the peeked direction.
 * @note If the queue is empty, no direction is returned.
 * @note If any param is null nothing happens.
 */
void queue_peek_last(Queue *queue, Direction *dir) {
  if (queue == NULL || dir == NULL) return;
  taskENTER_CRITICAL(&queue_mux);
  if (queue->occupied == 0) {
    taskEXIT_CRITICAL(&queue_mux);
    return;
  }
  size_t last_index = (queue->tail + QUEUE_SIZE - 1) % QUEUE_SIZE;
  *dir = queue->q[last_index];
  taskEXIT_CRITICAL(&queue_mux);
}

/**
 * @brief Clears the queue, resetting head, tail, and occupied count.
 * @param queue Pointer to the Queue structure.
 */
void queue_clear(Queue *queue) {
  if (queue == NULL) return;
  taskENTER_CRITICAL(&queue_mux);
  queue->head = 0;
  queue->tail = 0;
  queue->occupied = 0;
  taskEXIT_CRITICAL(&queue_mux);
}

/*******************************EOF dir_queue.c*******************************/