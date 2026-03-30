#ifndef HUD_H
#define HUD_H

#include "structures.h"

void draw_hud(Body* bodies, int count);
void toggle_hud();
void init_hud();

// clique
void hud_click(int mouse_x, int mouse_y);

#endif