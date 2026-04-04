#include <math.h>
#include "camera_controller.h"
#include "calculus.h"
#include "app_state.h"

float camera_zoom = 1.0f;
extern float distance_scale;
extern float radius_scale;
CameraMode camera_mode = CAMERA_FREE;

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

Position lerp_pos(Position a, Position b, float t) {
    return (Position){
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t)
    };
}

Position catmull_rom(Position p0, Position p1, Position p2, Position p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    Position res;
    res.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    res.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    res.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);
    return res;
}

void update_camera(float delta_time) {
    if (camera_mode == CAMERA_FREE) return;

    Position target;
    float dist = 300.0f;

    // 1. Determinar o Alvo (Target) e Distância Base
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

    // 2. Lógica de Movimentação
    Position desired_cam;

    if (camera_mode == CAMERA_ORBIT) {
        // --- LÓGICA DE SPLINE ---
        // Definimos 4 pontos ao redor do target no plano XZ
        Position p[4];
        float orbit_dist = dist * 1.2; // Um pouco mais longe para o modo orbit

        
        orbit_dist = dist * 1.2f; // Deixamos mais longe para os planetas

        p[0] = (Position){ target.x + orbit_dist, target.y, target.z };                
        p[1] = (Position){ target.x,              target.y, target.z + orbit_dist }; 
        p[2] = (Position){ target.x - orbit_dist, target.y, target.z };                
        p[3] = (Position){ target.x,              target.y, target.z - orbit_dist }; 
        

        // Usamos o tempo global para circular entre os pontos 0, 1, 2, 3
        float speed = 0.1f; // Velocidade da órbita
        float global_t = time_sim * speed;
        
        int i1 = ((int)global_t) % 4;
        int i0 = (i1 + 3) % 4;
        int i2 = (i1 + 1) % 4;
        int i3 = (i1 + 2) % 4;
        
        float local_t = global_t - (int)global_t;

        desired_cam = catmull_rom(p[i0], p[i1], p[i2], p[i3], local_t);
    } 
    else {
        // --- LÓGICA DE FOLLOW (Original) ---
        float len = sqrt(3.0f);
        Position offset = { dist/len, dist/len, dist/len };
        desired_cam = (Position){ target.x + offset.x, target.y + offset.y, target.z + offset.z };
    }

    // 3. Suavização Final
    float smooth_pos = (camera_mode == CAMERA_ORBIT) ? 0.05f : 0.08f;
    cam.lookFrom = lerp_pos(cam.lookFrom, desired_cam, smooth_pos);
    cam.lookAt   = lerp_pos(cam.lookAt, target, 0.12f);
}