/**
 * @file models.c
 * @brief Implementation of models, constants and configurations for the game
 * and application
 * @author Vít Mrkvica (xmrkviv00)
 * @date 18/12/2024
 */
#include "models.h"

// GAME CONSTANTS
const Dir DIR_DELTA[4] = {[DIR_UP] = {{-1, 0}, DIR_UP, DIR_DOWN},
                          [DIR_DOWN] = {{1, 0}, DIR_DOWN, DIR_UP},
                          [DIR_LEFT] = {{0, -1}, DIR_LEFT, DIR_RIGHT},
                          [DIR_RIGHT] = {{0, 1}, DIR_RIGHT, DIR_LEFT}};

const Dif DIFFICULTIES[3] = {[DIFF_EASY] = {.name = DIFF_EASY,
                                            .move_T = 5,
                                            .food_T = 60,
                                            .evil_food_T = 120,
                                            .food_spawn_chance = 80,
                                            .evil_food_spawn_chance = 20,
                                            .min_snake_len = 4,
                                            .max_fruit = 3,
                                            .max_evil_fruit = 1,
                                            .fruit_ttl = 300,
                                            .evil_fruit_ttl = 130,
                                            .winning_len = 30,
                                            .good_inc = 1,
                                            .evil_dec = 1},
                             [DIFF_MEDIUM] = {.name = DIFF_MEDIUM,
                                              .move_T = 3,
                                              .food_T = 60,
                                              .evil_food_T = 70,
                                              .food_spawn_chance = 60,
                                              .evil_food_spawn_chance = 40,
                                              .max_fruit = 5,
                                              .max_evil_fruit = 5,
                                              .fruit_ttl = 300,
                                              .evil_fruit_ttl = 360,
                                              .min_snake_len = 5,
                                              .winning_len = 40,
                                              .good_inc = 2,
                                              .evil_dec = 2},
                             [DIFF_HARD] = {.name = DIFF_HARD,
                                            .move_T = 2,
                                            .food_T = 60,
                                            .evil_food_T = 70,
                                            .food_spawn_chance = 40,
                                            .evil_food_spawn_chance = 60,
                                            .max_fruit = 4,
                                            .max_evil_fruit = 6,
                                            .fruit_ttl = 240,
                                            .evil_fruit_ttl = 390,
                                            .min_snake_len = 6,
                                            .winning_len = 60,
                                            .good_inc = 3,
                                            .evil_dec = 5}};

const size_t MAX_GAME_ARRAY_LEN = ROWS * COLS;
const size_t MIN_GAME_ARRAY_LEN = 0;

// COLOR CONFIG
const rgb16_t SNAKE_COLOR = {2000, 0, 4095};
const rgb16_t SNAKE_HEAD_COLOR = {200, 0, 4095};
const rgb16_t FRUIT_COLOR = {0, 4095, 80};
const rgb16_t EVIL_FRUIT_COLOR = {4095, 80, 0};
const rgb16_t TEXT_COLOR = {4095, 0, 4095};
const rgb16_t EASY_COLOR = {0, 4095, 0};
const rgb16_t MEDIUM_COLOR = {3000, 1000, 0};
const rgb16_t HARD_COLOR = {4095, 0, 0};
const rgb16_t SELECTED_COLOR = {4095, 0, 4095};
const rgb16_t LOST_COLOR = {4095, 0, 0};
const rgb16_t WON_COLOR = {4095, 4095, 0};

/**************************************EOF
 * models.c*************************************/