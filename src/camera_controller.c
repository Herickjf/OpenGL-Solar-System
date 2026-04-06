/*
 * camera_controller.c — Modos CAMERA_FOLLOW (interpolação para trás do alvo) e CAMERA_ORBIT
 * (posição ao longo de spline Catmull–Rom em torno do alvo). CAMERA_FREE não altera a câmera aqui.
 */
#include <math.h>
#include <GL/glut.h>
#include "camera_controller.h"
#include "calculus.h"
#include "app_state.h"

float camera_zoom = 1.0f;
extern float distance_scale;
extern float radius_scale;
CameraMode camera_mode = CAMERA_FREE;

// --- Interpolação e spline ---

/**
 * @brief Realiza uma interpolação linear simples (LERP) entre dois valores.
 * @param a Valor inicial.
 * @param b Valor final.
 * @param t Fator de interpolação (geralmente entre 0.0 e 1.0).
 * @return O valor interpolado entre a e b.
 */
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief Realiza a interpolação linear entre dois vetores de posição.
 * @param a Posição inicial.
 * @param b Posição final.
 * @param t Fator de interpolação.
 * @return Uma nova estrutura Position resultante da interpolação.
 */
Position lerp_pos(Position a, Position b, float t) {
    return (Position){
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t)
    };
}

/**
 * @brief Calcula um ponto em uma spline de Catmull-Rom.
 * Utilizada para criar trajetórias curvas suaves entre pontos de controle.
 * @param p0 Ponto de controle anterior.
 * @param p1 Ponto de controle atual (início do segmento).
 * @param p2 Ponto de controle próximo (fim do segmento).
 * @param p3 Ponto de controle seguinte.
 * @param t Parâmetro de tempo local do segmento (0.0 a 1.0).
 * @return A posição calculada sobre a curva.
 */
Position catmull_rom(Position p0, Position p1, Position p2, Position p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    Position res;
    res.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    res.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    res.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);
    return res;
}

// --- Atualização por frame (chamada desde main.c / temporizador) ---

/**
 * @brief Atualiza a posição e o foco da câmera com base no modo de operação (FOLLOW ou ORBIT).
 * Calcula o alvo (planeta ou lua), define a distância ideal e aplica suavização (suavização por interpolação)
 * na movimentação para garantir transições fluidas. No modo ORBIT, utiliza splines para circundar o alvo.
 * @param delta_time Tempo decorrido desde o último frame (não utilizado diretamente nesta implementação).
 */
void update_camera(float delta_time) {
    if (camera_mode == CAMERA_FREE) return;

    Position target;
    float dist = 300.0f;

    if (focused_body) {
        target = get_position(focused_body);
        dist = focused_body->radius * radius_scale * 6.0f * camera_zoom;
    } 
    else if (focused_moon) {
        Position moon_rel = get_moon_position(focused_moon);
        Position parent_pos = {0,0,0};
        if (moon_parent) parent_pos = get_position(moon_parent);

        target = (Position){
            parent_pos.x + moon_rel.x,
            parent_pos.y + moon_rel.y,
            parent_pos.z + moon_rel.z
        };
        dist = focused_moon->orbit_radius * distance_scale * 2.5f * camera_zoom;
    } else return;

    Position desired_cam;

    if (camera_mode == CAMERA_ORBIT) {
        float orbit_dist = dist * 1.2f;

        // Pontos de controle da Spline
        Position p[4];
        p[0] = (Position){ target.x + orbit_dist, target.y, target.z };                
        p[1] = (Position){ target.x,               target.y, target.z + orbit_dist }; 
        p[2] = (Position){ target.x - orbit_dist, target.y, target.z };                
        p[3] = (Position){ target.x,               target.y, target.z - orbit_dist }; 

        float speed = 0.1f;
        float global_t = time_sim * speed;
        int i1 = ((int)global_t) % 4;
        int i0 = (i1 + 3) % 4;
        int i2 = (i1 + 1) % 4;
        int i3 = (i1 + 2) % 4;
        float local_t = global_t - (int)global_t;

        Position raw_spline_pos = catmull_rom(p[i0], p[i1], p[i2], p[i3], local_t);

        // --- ANTI-PULSAÇÃO (NORMALIZAÇÃO) ---
        float dx = raw_spline_pos.x - target.x;
        float dy = raw_spline_pos.y - target.y;
        float dz = raw_spline_pos.z - target.z;
        float current_dist = sqrt(dx*dx + dy*dy + dz*dz);

        // Força a distância a ser sempre orbit_dist
        desired_cam.x = target.x + (dx / current_dist) * orbit_dist;
        desired_cam.y = target.y + (dy / current_dist) * orbit_dist;
        desired_cam.z = target.z + (dz / current_dist) * orbit_dist;
    } 
    else {
        float len = sqrt(3.0f);
        Position offset = { dist/len, dist/len, dist/len };
        desired_cam = (Position){ target.x + offset.x, target.y + offset.y, target.z + offset.z };
    }

    float smooth_pos = (camera_mode == CAMERA_ORBIT) ? 0.1f : 0.08f; // Aumentado para 0.1 no orbit para mais firmeza
    cam.lookFrom = lerp_pos(cam.lookFrom, desired_cam, smooth_pos);
    cam.lookAt   = lerp_pos(cam.lookAt, target, 0.12f);
}