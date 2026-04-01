#ifndef APP_STATE_H
#define APP_STATE_H

    #include <GL/glut.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include "./structures.h"

    extern Camera cam;
    extern int camera_mode;
    extern float camera_zoom;

    extern float time_scale;

    // HUD
    extern int show_hud;

    // foco
    extern Body* focused_body;
    extern Moon* focused_moon;
    extern Body* moon_parent;


    // tempo pausado
    extern int is_paused;
    
#endif
