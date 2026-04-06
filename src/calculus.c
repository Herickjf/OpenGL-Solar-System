/*
 * calculus.c — Posição 3D dos corpos em órbita elíptica simplificada (excentricidade, inclinação)
 * em função do tempo simulado time_sim e de distance_scale.
 */
#include <math.h>
#include "calculus.h"

// =========================
// Planetas

/**
 * @brief Calcula a posição 3D de um planeta em sua órbita elíptica.
 * Utiliza a equação da cônica em coordenadas polares considerando o raio orbital, 
 * excentricidade e inclinação para determinar as coordenadas (x, y, z) em função do tempo.
 * @param body Ponteiro para a estrutura do corpo celeste (Body).
 * @return Uma estrutura Position contendo as coordenadas X, Y e Z do planeta.
 */
Position get_position(Body* body) {
    // Sol fica parado
    if (body->orbit_radius == 0) {
        return (Position){0, 0, 0};
    }

    float angle = time_sim * (2.0f * M_PI / body->orbital_period);

    float a = body->orbit_radius * distance_scale;
    float e = body->eccentricity;

    float r = a * (1 - e*e) / (1 + e * cos(angle));

    float x = r * cos(angle);
    float z = r * sin(angle);

    float inc = body->orbit_inclination * M_PI / 180.0f;

    float y_rot = z * sin(inc);
    float z_rot = z * cos(inc);

    return (Position){
        x,
        y_rot,
        z_rot
    };
}

// =========================
// Luas (órbita relativa; posição absoluta combina-se com o planeta em draw/câmera)

/**
 * @brief Calcula a posição 3D relativa de uma lua em relação ao seu planeta pai.
 * Segue o mesmo modelo matemático de órbita elíptica (Kepleriana simplificada) 
 * aplicado aos planetas, utilizando os parâmetros específicos da lua.
 * @param moon Ponteiro para a estrutura da lua (Moon).
 * @return Uma estrutura Position contendo as coordenadas X, Y e Z relativas ao centro do planeta.
 */
Position get_moon_position(Moon* moon) {
    float angle = time_sim * (2.0f * M_PI / moon->orbital_period);

    float a = moon->orbit_radius * distance_scale;
    float e = moon->eccentricity;

    float r = a * (1 - e*e) / (1 + e * cos(angle));

    float x = r * cos(angle);
    float z = r * sin(angle);

    float inc = moon->orbit_inclination * M_PI / 180.0f;

    float y_rot = z * sin(inc);
    float z_rot = z * cos(inc);

    return (Position){
        x,
        y_rot,
        z_rot
    };
}