#ifndef __CALCULUS_H__
#define __CALCULUS_H__

#include "structures.h"

// variáveis globais
extern float time_sim;
extern float distance_scale;

// funções
Position get_position(Body* body);
Position get_moon_position(Moon* moon);

#endif