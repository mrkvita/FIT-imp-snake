#ifndef MY_UTILSZ_H
#define MY_UTILSZ_H
#include "models.h"

void insert_dir(Direction dir, GameManager *gm);
bool conflictDir(Direction d, Direction last, GameManager *gm);
int rand_range(int min, int max);

#endif