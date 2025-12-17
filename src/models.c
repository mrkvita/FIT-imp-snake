#include "models.h"

// GLOBAL CONSTANTS
const Dir DIR_DELTA[4] = {[DIR_UP] = {{-1, 0}, DIR_UP, DIR_DOWN},
                          [DIR_DOWN] = {{1, 0}, DIR_DOWN, DIR_UP},
                          [DIR_LEFT] = {{0, -1}, DIR_LEFT, DIR_RIGHT},
                          [DIR_RIGHT] = {{0, 1}, DIR_RIGHT, DIR_LEFT}};

const Dif DIFFICULTIES[3] = {[DIFF_EASY] = {.name = DIFF_EASY,
                                            .speed = 5,
                                            .food_freq = 1000,
                                            .max_fruit = 3,
                                            .max_evil_fruit = 0,
                                            .min_snake_len = 3,
                                            .good_inc = 2,
                                            .evil_dec = 3},
                             [DIFF_MEDIUM] = {.name = DIFF_MEDIUM,
                                              .speed = 8,
                                              .food_freq = 800,
                                              .max_fruit = 2,
                                              .max_evil_fruit = 1,
                                              .min_snake_len = 3,
                                              .good_inc = 2,
                                              .evil_dec = 1},
                             [DIFF_HARD] = {.name = DIFF_HARD,
                                            .speed = 12,
                                            .food_freq = 500,
                                            .max_fruit = 1,
                                            .max_evil_fruit = 2,
                                            .min_snake_len = 3,
                                            .good_inc = 3,
                                            .evil_dec = 2}};

// COLOR CONFIG
const rgb16_t SNAKE_COLOR = {4095, 4095, 0};
const rgb16_t FRUIT_COLOR = {0, 4095, 0};
const rgb16_t EVIL_FRUIT_COLOR = {4095, 0, 0};
const rgb16_t TEXT_COLOR = {4095, 4095, 4095};
const rgb16_t EASY_COLOR = {0, 4095, 0};
const rgb16_t MEDIUM_COLOR = {4095, 4095, 0};
const rgb16_t HARD_COLOR = {4095, 0, 0};
const rgb16_t SELECTED_COLOR = {0, 0, 4095};