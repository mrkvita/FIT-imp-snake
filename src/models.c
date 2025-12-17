#include "models.h"

const Dir DIR_DELTA[4] = {[DIR_UP] = {{-1, 0}, DIR_UP, DIR_DOWN},
                          [DIR_DOWN] = {{1, 0}, DIR_DOWN, DIR_UP},
                          [DIR_LEFT] = {{0, -1}, DIR_LEFT, DIR_RIGHT},
                          [DIR_RIGHT] = {{0, 1}, DIR_RIGHT, DIR_LEFT}};

const Dif DIFFICULTIES[3] = {[DIFF_EASY] = {.speed = 5,
                                            .food_freq = 1000,
                                            .max_fruit = 3,
                                            .max_evil_fruit = 0,
                                            .min_snake_len = 3,
                                            .good_inc = 2,
                                            .evil_dec = 3},
                             [DIFF_MEDIUM] = {.speed = 8,
                                              .food_freq = 800,
                                              .max_fruit = 2,
                                              .max_evil_fruit = 1,
                                              .min_snake_len = 3,
                                              .good_inc = 2,
                                              .evil_dec = 1},
                             [DIFF_HARD] = {.speed = 12,
                                            .food_freq = 500,
                                            .max_fruit = 1,
                                            .max_evil_fruit = 2,
                                            .min_snake_len = 3,
                                            .good_inc = 3,
                                            .evil_dec = 2}};
