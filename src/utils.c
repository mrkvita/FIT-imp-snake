/**
 * @file utils.c
 * @brief Implementation of utils functions, that don't belong anywhere.
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024
 */
#include "utils.h"

#include "dir_queue.h"
#include "esp_random.h"
#include "globals.h"
#include "models.h"

/**
 * @brief Checks if the given direction conflicts with the last direction in
 * the queue or the snake's current direction.
 * @param d The new direction to check.
 * @param last The last direction in the queue.
 * @param gm Pointer to the GameManager structure.
 * @return true if there is a conflict, false otherwise.
 */
bool conflictDir(Direction d, Direction last, GameManager *gm) {
  if (gm == NULL) return true;
  if (last == DIR_EMPTY) {
    if (gm->snake.dir.name == d ||
        gm->snake.dir.opposite == d) {  // conflict with snake
      return true;
    }
  }
  if (last != DIR_EMPTY) {
    if (d == last || d == DIR_DELTA[last].opposite) {
      return true;
    }
  }
  return false;
};

/**
 * @brief Inserts a new direction into the direction queue if it does not
 * conflict with the last direction or the snake's current direction.
 * @param dir The new direction to insert.
 * @param gm Pointer to the GameManager structure.
 */
void insert_dir(Direction dir,
                GameManager *gm) {  // only inserts the direction if allowed
  if (gm == NULL) return;
  Direction last_dir = DIR_EMPTY;
  queue_peek_last(&direction, &last_dir);
  if (!conflictDir(dir, last_dir, gm)) {
    queue_push(&direction, dir);
  }
}

/**
 * @brief Generates a random integer within the specified range [min, max].
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 * @return A random integer between min and max (inclusive).
 */
int rand_range(int min, int max) {
  return esp_random() % (max - min + 1) + min;
}

/********************************EOF utils.c*********************************/