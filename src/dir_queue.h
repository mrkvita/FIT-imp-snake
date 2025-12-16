#ifndef MY_DIR_QUEUE_H
#define MY_DIR_QUEUE_H

#include "models.h"

void queue_push(Queue *queue, Direction dir);

void queue_pop(Queue *queue, Direction *dir);

void queue_peek(Queue *queue, Direction *dir);

void queue_peek_last(Queue *queue, Direction *dir);
#endif
