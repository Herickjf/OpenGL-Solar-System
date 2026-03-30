#ifndef DRAW_H
#define DRAW_H

#include "structures.h"

// precisa acessar variáveis globais
extern float radius_scale;
extern float time_sim;

// funções de desenho
void drawBackground();
void drawSun(Body *sun);
void drawPlanet(Body *planet);
void drawMoon(Moon *moon);

#endif