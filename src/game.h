#ifndef MY_GAME_H
#define MY_GAME_H

#include "models.h"
#include <stdbool.h>

bool is_colision(Pos *a, Pos *b);
bool collision_detected(GameManager *gm);
bool food_eaten(GameManager *gm, bool *is_evil);
size_t get_free_index(GameManager *gm);
bool should_spawn_fruit(GameManager *gm);
void remove_expired_fruits(GameManager *gm);
bool should_spawn_evil_fruit(GameManager *gm);
Pos get_pos(GameManager *gm, bool *_valid);
Difficulty get_next_difficulty(Difficulty current);
Difficulty get_prev_difficulty(Difficulty current);
State check_conditions(GameManager *gm);
void spawn_fruit(GameManager *gm);



#endif