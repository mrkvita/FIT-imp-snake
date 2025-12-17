#ifndef MY_DRAW_H
#define MY_DRAW_H
#include "models.h"

void fb_clear(); 
void fb_swap(); 
void draw_won();
void draw_lost();
void draw_idle(GameManager *gm); 
void draw_running(GameManager *gm);


#endif