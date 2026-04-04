/*
g++ main.c src/bodies.c src/hud.c libs/cJSON.c src/utils.c src/calculus.c src/input.c src/draw.c src/stb_image.c -Iinclude -o solarSystem -lGL -lGLU -lglut -lGLEW && ./solarSystem 
*/

#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "app_state.h"
#include "input.h"
#include "bodies.h"
#include "draw.h"
#include "hud.h"
#include "calculus.h"

// ======================
// FPS
// ======================
int last_frame_time = 0;
int target_fps = 30;
int frame_duration_ms = 1000 / target_fps;

// ======================
// escala
// ======================
float distance_scale;
float radius_scale;
float time_scale;

// ======================
// corpos
// ======================
int body_count = 0;
Body *bodies = NULL;

// ======================
// câmera
// ======================
Camera cam = {
    {0.0f, 800.0f, 2500.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
};

float camera_zoom = 1.0f;
// foco (GLOBAL)
Body* focused_body = NULL;
Moon* focused_moon = NULL;
Body* moon_parent = NULL;

// ======================
// tempo
// ======================
float time_sim = 0.0f;
int last_time = 0;

GLUquadric *quad;

// ======================
// FORWARD DECLARATIONS
// ======================
void update_timer(int);

// ======================
// UTIL: LERP
// ======================
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

// ======================
// CAMERA FOLLOW
// ======================
void update_camera_follow() {
    Position target;
    float dist = 300.0f;

    // ======================
    // PLANETA
    // ======================
    if (focused_body) {
        target = get_position(focused_body);

        dist = focused_body->radius * radius_scale * 6.0f * camera_zoom;
    }

    // ======================
    // LUA
    // ======================
    else if (focused_moon) {

        Position moon_rel = get_moon_position(focused_moon);

        Position parent_pos = {0,0,0};

        if (moon_parent) {
            parent_pos = get_position(moon_parent);
        }

        target = (Position){
            parent_pos.x + moon_rel.x,
            parent_pos.y + moon_rel.y,
            parent_pos.z + moon_rel.z
        };

        // distância proporcional à órbita (muito melhor)
        dist = focused_moon->orbit_radius * distance_scale * 2.5f * camera_zoom;
    }
    else {
        return;
    }

    // ======================
    // direção da câmera (melhor visual)
    // ======================
    float len = sqrt(3.0f);

    Position offset = {
        dist * (1.0f / len),
        dist * (1.0f / len),
        dist * (1.0f / len)
    };

    Position desired_cam = {
        target.x + offset.x,
        target.y + offset.y,
        target.z + offset.z
    };

    // ======================
    // suavização mais estável
    // ======================
    float smooth_pos = 0.08f;
    float smooth_look = 0.12f;

    cam.lookFrom = lerp_pos(cam.lookFrom, desired_cam, smooth_pos);
    cam.lookAt   = lerp_pos(cam.lookAt, target, smooth_look);
}

// ======================
// UPDATE
// ======================
void update() {
    int current_time = glutGet(GLUT_ELAPSED_TIME);

    float delta = (current_time - last_time) / 1000.0f;
    last_time = current_time;

    time_sim += delta * time_scale;

    // câmera segue foco
    update_camera_follow();

    if (current_time - last_frame_time >= frame_duration_ms) {
        last_frame_time = current_time;
        glutPostRedisplay();
    }

    glutTimerFunc(1, update_timer, 0);
}

void update_timer(int value) {
    update();
}

// ======================
// INIT OPENGL
// ======================
void init(void)
{
    GLfloat light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

   glClearColor(0,0,0,0);
   glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, scene_ambient);

    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   quad = gluNewQuadric();
   gluQuadricTexture(quad, GL_TRUE);
   gluQuadricNormals(quad, GLU_SMOOTH);
}

// ======================
// DISPLAY
// ======================
void display(void)
{
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
            if (bodies[i].orbit_radius > 0) {
                draw_orbit(&bodies[i]);
            }
        }

        for (int i = 1; i < body_count; i++) {
            drawPlanet(&bodies[i]);
        }

    glPopMatrix();

    // HUD por cima
    draw_hud(bodies, body_count);

    glutSwapBuffers();
}

// ======================
// RESHAPE
// ======================
void reshape(int w, int h){
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        60.0,
        (float)w / (float)h,
        1.0,
        20000.0
    );

    glMatrixMode(GL_MODELVIEW);
}

// ======================
// MAIN
// ======================
int main(int argc, char **argv)
{
    bodies = load_bodies("configs.json", &body_count);

    if (body_count == 0) {
        printf("Erro: Nenhum planeta lido\n");
        exit(1);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Solar System");

    glewInit();

    last_time = glutGet(GLUT_ELAPSED_TIME);

    init();
    init_hud();
    init_camera_controller();

    load_all_textures(bodies, body_count);

    // callbacks
    glutTimerFunc(0, update_timer, 0);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}