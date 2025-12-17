#include "models.h"

// GAME CONSTANTS
const Dir DIR_DELTA[4] = {[DIR_UP] = {{-1, 0}, DIR_UP, DIR_DOWN},
                          [DIR_DOWN] = {{1, 0}, DIR_DOWN, DIR_UP},
                          [DIR_LEFT] = {{0, -1}, DIR_LEFT, DIR_RIGHT},
                          [DIR_RIGHT] = {{0, 1}, DIR_RIGHT, DIR_LEFT}};

const Dif DIFFICULTIES[3] = {[DIFF_EASY] = {.name = DIFF_EASY,
                                            .speed = 5,
                                            .food_T = 10,
                                            .max_fruit = 3,
                                            .max_evil_fruit = 0,
                                            .min_snake_len = 3,
                                            .fruit_ttl = 40,
                                            .evil_fruit_ttl = 20,
                                            .winning_len = 10,
                                            .good_inc = 2,
                                            .evil_dec = 0},
                             [DIFF_MEDIUM] = {.name = DIFF_MEDIUM,
                                              .speed = 8,
                                              .food_T = 30,
                                              .max_fruit = 2,
                                              .max_evil_fruit = 1,
                                              .fruit_ttl = 40,
                                              .evil_fruit_ttl = 40,
                                              .min_snake_len = 3,
                                              .winning_len = 15,
                                              .good_inc = 2,
                                              .evil_dec = 1},
                             [DIFF_HARD] = {.name = DIFF_HARD,
                                            .speed = 12,
                                            .food_T = 30,
                                            .max_fruit = 1,
                                            .max_evil_fruit = 2,
                                            .fruit_ttl = 10,
                                            .evil_fruit_ttl = 40,
                                            .min_snake_len = 3,
                                            .winning_len = 20,
                                            .good_inc = 3,
                                            .evil_dec = 2}};

const size_t MAX_GAME_ARRAY_LEN = ROWS * COLS;

// COLOR CONFIG
const rgb16_t SNAKE_COLOR = {4095, 4095, 0};
const rgb16_t SNAKE_HEAD_COLOR = {3000, 1000 , 0};
const rgb16_t FRUIT_COLOR = {0, 4095, 0};
const rgb16_t EVIL_FRUIT_COLOR = {4095, 0, 0};
const rgb16_t TEXT_COLOR = {100, 100,100};
const rgb16_t EASY_COLOR = {0, 4095, 0};
const rgb16_t MEDIUM_COLOR = {3000, 1000, 0};
const rgb16_t HARD_COLOR = {4095, 0, 0};
const rgb16_t SELECTED_COLOR = {0, 0, 4095};
const rgb16_t LOST_COLOR = {4095, 0, 0};
const rgb16_t WON_COLOR =  {4095, 4095, 0};
