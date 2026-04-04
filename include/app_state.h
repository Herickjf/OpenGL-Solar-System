#ifndef APP_STATE_H
#define APP_STATE_H

    #include <GL/glut.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "./structures.h"

    // --- ESCALAS ---
    extern float distance_scale;
    extern float radius_scale;
    extern float time_scale;

    // --- ESTADO DA SIMULAÇÃO ---
    extern float time_sim;
    extern int body_count;
    extern Body *bodies;

    // --- CÂMERA E FOCO ---
    extern Camera cam;
    extern float camera_zoom;
    extern Body* focused_body;
    extern Moon* focused_moon;
    extern Body* moon_parent;
    extern CameraMode camera_mode;


    // tempo pausado
    extern int is_paused;
    extern int show_hud;

    // música
    extern int pause_music;
    
#endif
