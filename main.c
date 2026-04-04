/*
g++ main.c src/bodies.c src/hud.c src/audio_controller.c src/camera_controller.c libs/cJSON.c src/utils.c src/calculus.c src/input.c src/draw.c src/stb_image.c -Iinclude -o solarSystem -lGL -lGLU -lglut -lGLEW -lSDL2 -lSDL2_mixer && ./solarSystem 
*/

#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_state.h"
#include "input.h"
#include "bodies.h"
#include "draw.h"
#include "hud.h"
#include "calculus.h"
#include "camera_controller.h"
#include "audio_controller.h"

int last_frame_time = 0;
int target_fps = 30;
int frame_duration_ms = 1000 / target_fps;

int body_count = 0;
Body *bodies = NULL;
Camera cam = {{0.0f, 800.0f, 2500.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
Body* focused_body = NULL;
Moon* focused_moon = NULL;
Body* moon_parent = NULL;

float distance_scale = 1.0f;
float radius_scale = 1.0f;
float time_scale = 1.0f;
float time_sim = 0.0f;
int last_time = 0;
int show_hud = 1;
GLUquadric *quad;

void update_timer(int value) {
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    float delta = (current_time - last_time) / 1000.0f;
    last_time = current_time;

    time_sim += delta * time_scale;

    // Chamada encapsulada do controlador
    update_camera(delta);
    update_audio();

    if (current_time - last_frame_time >= frame_duration_ms) {
        last_frame_time = current_time;
        glutPostRedisplay();
    }
    glutTimerFunc(1, update_timer, 0);
}

void init(void) {
    GLfloat mat_specular[] = {1,1,1,1};
    GLfloat mat_shininess[] = {50};
    GLfloat light_position[] = {1,1,1,0};

    glClearColor(0,0,0,0);
    glShadeModel(GL_SMOOTH);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        cam.lookFrom.x, cam.lookFrom.y, cam.lookFrom.z,
        cam.lookAt.x,   cam.lookAt.y,   cam.lookAt.z,
        cam.vUp.x,      cam.vUp.y,      cam.vUp.z
    );

    GLfloat light_position[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glPushMatrix();
        drawBackground();
        drawSun(&bodies[0]);
        for (int i = 0; i < body_count; i++) {
            if (bodies[i].orbit_radius > 0) draw_orbit(&bodies[i]);
        }
        for (int i = 1; i < body_count; i++) {
            drawPlanet(&bodies[i]);
        }
    glPopMatrix();

    draw_hud(bodies, body_count);
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w / (float)h, 1.0, 20000.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
    bodies = load_bodies("configs.json", &body_count);
    if (body_count == 0) exit(1);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Solar System");

    glewInit();
    last_time = glutGet(GLUT_ELAPSED_TIME);

    init();
    init_hud();
    init_camera_controller();
    init_audio_controller();
    load_all_textures(bodies, body_count);

    glutTimerFunc(0, update_timer, 0);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}