/**
 * @file game.c
 * @brief Implementation of game logic functions, utilities and helpers
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024
 */
#include "game.h"

#include <stdbool.h>

#include "dir_queue.h"
#include "models.h"
#include "tlc5947.h"  // for rows and cols
#include "utils.h"

/**
 * @brief Checks if two positions collide (are the same).
 *
 * @param a Pointer to first position.
 * @param b Pointer to second position.
 * @return true if positions are the same, false otherwise.
 */
bool is_collision(Pos *a, Pos *b) {
  if (a == NULL || b == NULL) return false;
  return (a->r == b->r) && (a->c == b->c);
}

/**
 * @brief Checks if the snake collides with itself.
 * @param gm Pointer to the GameManager structure.
 * @return true if collision is detected, false otherwise.
 */
bool collision_detected(GameManager *gm) {
  if (gm == NULL) {
    return false;
  }
  Pos head = gm->snake.body[0];
  for (size_t i = 1; i < gm->snake.len; i++) {
    if (is_collision(&head, &gm->snake.body[i])) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Checks if the snake has eaten a fruit.
 * @param gm Pointer to the GameManager structure.
 * @param is_evil Pointer to a boolean that will be set to true if the eaten
 * fruit is evil.
 * @return true if a fruit was eaten, false otherwise. Invariant towards is_evil
 * if no fruit is eaten or gm is NULL.
 */
bool food_eaten(GameManager *gm, bool *is_evil) {
  if (gm == NULL) {
    return false;
  }
  Pos head = gm->snake.body[0];
  for (size_t i = 0; i < MAX_GAME_ARRAY_LEN; i++) {  // check all fruits
    if (!gm->fruits[i].enabled) continue;           // only check enabled fruits
    if (is_collision(&head, &gm->fruits[i].pos)) {  // collision with fruit
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
    }  // collision with fruit
  }    // all fruits
  return false;
}

/**
 * @brief Finds a free index in the fruits array of the GameManager.
 * @param gm Pointer to the GameManager structure.
 * @return Index of a free slot in the fruits array. If no free slot is found,
 * returns MIN_GAME_ARRAY_LEN even if it is occupied.
 */
size_t get_free_index(GameManager *gm) {
  if (gm == NULL) {
    return MIN_GAME_ARRAY_LEN;
  }
  size_t i = 0;  // if the array is full the last index will be returned (this
                 // will never happen so whatever)
  for (; i < MAX_GAME_ARRAY_LEN; ++i) {
    if (!gm->fruits[i].enabled) {
      return i;
    }
  }
  return MIN_GAME_ARRAY_LEN;
}

/**
 * @brief Removes expired fruits from the game.
 * @param gm Pointer to the GameManager structure.
 */
void remove_expired_fruits(GameManager *gm) {
  if (gm == NULL) {
    return;
  }
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

/**
 * @brief Determines whether a new fruit should be spawned based on the game
 * state.
 * @param gm Pointer to the GameManager structure.
 * @return true if a new fruit should be spawned, false otherwise.
 */
bool should_spawn_fruit(GameManager *gm) {
  if (gm == NULL) {
    return false;
  }
  static int frame_counter = 0;  // the fruit has a period in game ticks
  frame_counter++;
  if (frame_counter >= gm->difficulty.food_T) {
    frame_counter = 0;
    if (gm->fruit_count < gm->difficulty.max_fruit) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Determines whether a new evil fruit should be spawned based on the
 * game state.
 * @param gm Pointer to the GameManager structure.
 * @return true if a new evil fruit should be spawned, false otherwise.
 */
bool should_spawn_evil_fruit(GameManager *gm) {
  if (gm == NULL) {
    return false;
  }
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

/**
 * @brief Generates a random valid position for a new fruit.
 * @param gm Pointer to the GameManager structure.
 * @param _valid Pointer to a boolean that will be set to true if a valid
 * position was found, false otherwise.
 * @return A valid position for a new fruit. If no valid position is found,
 * returns (0,0) and sets _valid to false. If gm or _valid is NULL, returns
 * (0,0) and invariant towards _valid.
 */
Pos get_pos(GameManager *gm, bool *_valid) {
  if (gm == NULL || _valid == NULL) {
    return (Pos){0, 0};  // invariant towards _valid
  }
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

/**
 * @brief Gets the next difficulty level in a circular manner.
 * @param current The current difficulty level.
 * @return The next difficulty level.
 */
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

/**
 * @brief Gets the previous difficulty level in a circular manner.
 * @param current The current difficulty level.
 * @return The previous difficulty level.
 */
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

/**
 * @brief Checks the game conditions to determine if the game is won, lost,
 * or still running.
 * @param gm Pointer to the GameManager structure.
 * @return Returns the appropriate game state change based on current
 * conditions. (GAME_RUNNING, GAME_WON, GAME_LOST)
 */
State check_conditions(GameManager *gm) {
  if (gm == NULL) {
    return GAME_RUNNING;
  }  // keep the presumably current state
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

/**
 * @brief Spawns new fruits in the game based on the current game state.
 * @param gm Pointer to the GameManager structure.
 */
void spawn_fruit(GameManager *gm) {
  if (gm == NULL) {
    return;
  }
  // spawn fruit
  if (should_spawn_fruit(gm) &&
      rand_range(0, 100) <
          gm->difficulty
              .food_spawn_chance) {  // its time to spawn fruit, roll for chance
    bool valid = false;
    Pos new_pos = get_pos(gm, &valid);  // get position
    if (valid) {                        // there may not be space for new fruit
      size_t free_index = get_free_index(gm);  // free index in the fruits array
      gm->fruits[free_index] = (Fruit){.pos = new_pos,  // spawn fruit
                                       .is_evil = false,
                                       .ttl = gm->difficulty.fruit_ttl,
                                       .enabled = true};
      gm->fruit_count++;
    }
  }
  if (should_spawn_evil_fruit(gm) &&
      rand_range(0, 100) <
          gm->difficulty.evil_food_spawn_chance) {  // its time to spawn evil
                                                    // fruit, roll for chance
    bool valid = false;
    Pos new_pos = get_pos(gm, &valid);  // get position
    if (valid) {                        // there may not be space for new fruit
      size_t free_index = get_free_index(gm);  // free index in the fruits array
      gm->fruits[free_index] = (Fruit){.pos = new_pos,  // spawn fruit
                                       .is_evil = true,
                                       .ttl = gm->difficulty.evil_fruit_ttl,
                                       .enabled = true};
      gm->evil_fruit_count++;
    }
  }
}

/**
 * @brief Moves the snake in the game based on the current direction queue.
 * Also handles snake growth or shrinkage based on buffered length.
 * @param gm Pointer to the GameManager structure.
 * @param direction Pointer to the direction queue.
 */
void move_snake(GameManager *gm, Queue *direction) {
  Direction next_dir = gm->snake.dir.name;
  queue_pop(direction, &next_dir);  // invariant if empty
  gm->snake.dir = DIR_DELTA[next_dir];

  // Increment snake from length buffer
  if (gm->buffered_len > 0) {
    if (gm->snake.len < MAX_GAME_ARRAY_LEN) {
      gm->snake.len++;  // means it will get redrawn at the end
    }
    gm->buffered_len--;
  }
  if (gm->buffered_len < 0) {
    if (gm->snake.len > MIN_GAME_ARRAY_LEN) {
      gm->snake.len--;
    }
    gm->buffered_len++;
  }

  for (int i = gm->snake.len - 1; i > 0; i--) {
    gm->snake.body[i] = gm->snake.body[i - 1];
  }
  // new head
  gm->snake.body[0].r += gm->snake.dir.pos.r;
  gm->snake.body[0].c += gm->snake.dir.pos.c;
  gm->snake.body[0].r = (gm->snake.body[0].r + ROWS) % ROWS;
  gm->snake.body[0].c = (gm->snake.body[0].c + COLS) % COLS;
}

/**********************************EOF game.c*********************************/