#include "game.h"
#include "models.h"
#include "utils.h"
#include <stdbool.h>


bool is_collision(Pos *a, Pos *b) { return (a->r == b->r) && (a->c == b->c); }

bool collision_detected(GameManager *gm) {
  Pos head = gm->snake.body[0];
  for (size_t i = 1; i < gm->snake.len; i++) {
    if (is_collision(&head, &gm->snake.body[i])) {
      return true;
    }
  }
  return false;
}

bool food_eaten(GameManager *gm, bool *is_evil) {
  Pos head = gm->snake.body[0];
  for (size_t i = 0; i < MAX_GAME_ARRAY_LEN; i++) {
    if (!gm->fruits[i].enabled) continue;
    if (is_collision(&head, &gm->fruits[i].pos)) {
      if (is_evil) {
        *is_evil = gm->fruits[i].is_evil;
      }

      // Remove fruit from array
      gm->fruits[i].enabled = false;
      if (gm->fruits[i].is_evil) {
        gm->evil_fruit_count--;
      } else {
        gm->fruit_count--;
      }

      return true;
    }
  }
  return false;
}

size_t get_free_index(GameManager *gm) {
  size_t i = 0;  // if the array is full the last index will be returned (this
                 // will never happen so whatever)
  for (; i < MAX_GAME_ARRAY_LEN; ++i) {
    if (!gm->fruits[i].enabled) {
      return i;
    }
  }
  return i;
}

bool should_spawn_fruit(GameManager *gm) {
  static int frame_counter = 0;
  frame_counter++;
  if (frame_counter >= gm->difficulty.food_T) {
    frame_counter = 0;
    if (gm->fruit_count < gm->difficulty.max_fruit) {
      return true;
    }
  }
  return false;
}

void remove_expired_fruits(GameManager *gm) {
  for (size_t i = 0; i < MAX_GAME_ARRAY_LEN; ++i) {
    if (!gm->fruits[i].enabled) continue;  // only modify enabled fruits

    gm->fruits[i].ttl--;            // decrease ttl
    if (gm->fruits[i].ttl <= 0) {   // fruit expired
      if (gm->fruits[i].is_evil) {  // decrease the correct counter
        gm->evil_fruit_count--;
      } else {
        gm->fruit_count--;
      }
      gm->fruits[i].enabled = false;  // disable the fruit
    }
  }
}

bool should_spawn_evil_fruit(GameManager *gm) {
  static int evil_frame_counter = 0;
  evil_frame_counter++;
  if (evil_frame_counter >= gm->difficulty.evil_food_T) {
    evil_frame_counter = 0;
    if (gm->evil_fruit_count < gm->difficulty.max_evil_fruit) {
      return true;
    }
  }
  return false;
}

Pos get_pos(GameManager *gm, bool *_valid) {
  bool found = false;
  int max_attempts = 1000;
  Pos new_pos = {};

  while (!found && max_attempts--) {
    new_pos.r = rand_range(0, ROWS - 1);
    new_pos.c = rand_range(0, COLS - 1);
    found = true;

    // snake collision
    for (size_t i = 0; i < gm->snake.len; ++i) {
      if (is_collision(&new_pos, &gm->snake.body[i])) {
        found = false;
        break;
      }
    }

    if (!found) continue;

    // fruit collision
    for (size_t i = 0; i < MAX_GAME_ARRAY_LEN; ++i) {
      if (!gm->fruits[i].enabled) continue;
      if (is_collision(&new_pos, &gm->fruits[i].pos)) {
        found = false;
        break;
      }
    }
  }
  *_valid = found;
  return new_pos;
}

Difficulty get_next_difficulty(Difficulty current) {
  if (current == DIFF_EASY) {
    return DIFF_MEDIUM;
  }
  if (current == DIFF_MEDIUM) {
    return DIFF_HARD;
  }
  if (current == DIFF_HARD) {
    return DIFF_EASY;
  }
  return current;
}

Difficulty get_prev_difficulty(Difficulty current) {
  if (current == DIFF_EASY) {
    return DIFF_HARD;
  }
  if (current == DIFF_MEDIUM) {
    return DIFF_EASY;
  }
  if (current == DIFF_HARD) {
    return DIFF_MEDIUM;
  }
  return current;
}

State check_conditions(GameManager *gm) {
  if (gm->snake.len < gm->difficulty.min_snake_len) {
    return GAME_LOST;
  }
  if (gm->snake.len > gm->difficulty.winning_len) {
    return GAME_WON;
  }
  if (collision_detected(gm)) {
    return GAME_LOST;
  }
  bool evil = false;
  if ((food_eaten(gm, &evil))) {
    if (evil) {
      gm->buffered_len -= gm->difficulty.evil_dec;
    } else {
      gm->buffered_len += gm->difficulty.good_inc;
    }
  }
  return GAME_RUNNING;
}

void spawn_fruit(GameManager *gm){
  // spawn fruit
  if (should_spawn_fruit(gm) &&
      rand() % 100 < gm->difficulty.food_spawn_chance) {
    bool valid = false;
    Pos new_pos = get_pos(gm, &valid);
    if (valid) {
      size_t free_index = get_free_index(gm);
      gm->fruits[free_index] = (Fruit){.pos = new_pos,
                                      .is_evil = false,
                                      .ttl = gm->difficulty.fruit_ttl,
                                      .enabled = true};
      gm->fruit_count++;
    }
  }
  if (should_spawn_evil_fruit(gm) &&
      rand() % 100 < gm->difficulty.evil_food_spawn_chance) {
    bool valid = false;
    Pos new_pos = get_pos(gm, &valid);
    if (valid) {
      size_t free_index = get_free_index(gm);
      gm->fruits[free_index] = (Fruit){.pos = new_pos,
                                      .is_evil = true,
                                      .ttl = gm->difficulty.evil_fruit_ttl,
                                      .enabled = true};
      gm->evil_fruit_count++;
    }
  }
}




