#ifndef __CALCULUS_H__
#define __CALCULUS_H__

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "app_state.h"
#include "bodies.h"

// time variables
extern float time_sim;

Position get_position(Body* body) {
    // Sol fica parado
    if (body->orbit_radius == 0) {
        return (Position){0, 0, 0};
    }

    // velocidade angular
    float angle = time_sim * (2.0f * M_PI / body->orbital_period);

    float a = body->orbit_radius * distance_scale;
    float e = body->eccentricity;

    // distância ao foco (Sol)
    float r = a * (1 - e*e) / (1 + e * cos(angle));

    float x = r * cos(angle);
    float z = r * sin(angle);

    // inclinação da órbita (rotação no eixo X)
    float inc = body->orbit_inclination * M_PI / 180.0f;

    float y_rot = z * sin(inc);
    float z_rot = z * cos(inc);

    return (Position){
        x,
        y_rot,
        z_rot
    };
}

Position get_moon_position(Moon* moon) {
    // velocidade angular
    float angle = time_sim * (2.0f * M_PI / moon->orbital_period);

    float a = moon->orbit_radius * distance_scale;
    float e = moon->eccentricity;

    // distância ao foco (Sol)
    float r = a * (1 - e*e) / (1 + e * cos(angle));

    float x = r * cos(angle);
    float z = r * sin(angle);

    // inclinação da órbita (rotação no eixo X)
    float inc = moon->orbit_inclination * M_PI / 180.0f;

    float y_rot = z * sin(inc);
    float z_rot = z * cos(inc);

    return (Position){
        x,
        y_rot,
        z_rot
    };
}


#endif