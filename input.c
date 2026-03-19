#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_state.h"
#include "input.h"

void keyboard(unsigned char key, int x, int y) {
    float camera_speed = 100.0f;

    (void)x;
    (void)y;

    switch (key) {
        case 'a':
        case 'A':
            cam.lookFrom.x -= camera_speed;
            cam.lookAt.x -= camera_speed;
            break;
        case 'd':
        case 'D':
            cam.lookFrom.x += camera_speed;
            cam.lookAt.x += camera_speed;
            break;
        case 'w':
        case 'W':
            cam.lookFrom.y += camera_speed;
            cam.lookAt.y += camera_speed;
            break;
        case 's':
        case 'S':
            cam.lookFrom.y -= camera_speed;
            cam.lookAt.y -= camera_speed;
            break;
        case '+':
            if (time_scale < 1600.0f) {
                time_scale *= 2.0f;
                printf("Time scale: %f\n", time_scale);
            }
            break;
        case '-':
            if (time_scale > 12.500f) {
                time_scale *= 0.5f;
                printf("Time scale: %f\n", time_scale);
            }
            break;
        case 27:
            exit(0);
            break;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    float scroll_speed = 200.0f;

    (void)x;
    (void)y;

    if (state == GLUT_DOWN) {
        if (button == 3) {
            cam.lookFrom.z += scroll_speed;
            cam.lookAt.z += scroll_speed;
        } else if (button == 4) {
            cam.lookFrom.z -= scroll_speed;
            cam.lookAt.z -= scroll_speed;
        }

        glutPostRedisplay();
    }
}
