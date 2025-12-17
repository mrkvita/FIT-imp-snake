#include "utils.h"

#include "dir_queue.h"
#include "globals.h"
#include "models.h"

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

void insert_dir(Direction dir, GameManager *gm) {  // only inserts the direction if allowed
  if (gm == NULL) return;
  Direction last_dir = DIR_EMPTY;
  queue_peek_last(&direction, &last_dir);
  if (!conflictDir(dir, last_dir, gm)) {
    queue_push(&direction, dir);
  }
}

int rand_range(int min, int max) { return rand() % (max - min + 1) + min; }