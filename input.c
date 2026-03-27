#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> // Necessário para sin() e cos()

#include "app_state.h"
#include "input.h"

// Variáveis de estado do mouse
static int is_dragging = 0;
static int last_mouse_x = 0;
static int last_mouse_y = 0;

// Ângulos da orientação da câmera (free look)
static float cam_yaw = 0.0f;
static float cam_pitch = 0.0f;

static Position normalize(Position v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) {
        return (Position){0.0f, 0.0f, -1.0f};
    }

    return (Position){v.x / len, v.y / len, v.z / len};
}

static Position cross(Position a, Position b) {
    return (Position){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

static Position get_forward_vector(void) {
    Position forward = {
        cosf(cam_pitch) * sinf(cam_yaw),
        sinf(cam_pitch),
        cosf(cam_pitch) * cosf(cam_yaw),
    };

    return normalize(forward);
}

static Position get_right_vector(Position forward) {
    Position right = cross(forward, cam.vUp);
    float len = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);

    if (len < 1e-6f) {
        return (Position){1.0f, 0.0f, 0.0f};
    }

    return (Position){right.x / len, right.y / len, right.z / len};
}

// Recalcula o lookAt com base na posição atual e na direção do olhar
void update_camera() {
    const float pitch_limit = 89.9f * (float)M_PI / 180.0f;
    const float look_distance = 1000.0f;
    Position forward;

    if (cam_pitch > pitch_limit) cam_pitch = pitch_limit;
    if (cam_pitch < -pitch_limit) cam_pitch = -pitch_limit;

    forward = get_forward_vector();
    cam.lookAt.x = cam.lookFrom.x + forward.x * look_distance;
    cam.lookAt.y = cam.lookFrom.y + forward.y * look_distance;
    cam.lookAt.z = cam.lookFrom.z + forward.z * look_distance;
}

void init_camera_controller(void) {
    Position direction = {
        cam.lookAt.x - cam.lookFrom.x,
        cam.lookAt.y - cam.lookFrom.y,
        cam.lookAt.z - cam.lookFrom.z,
    };

    direction = normalize(direction);
    cam_pitch = asinf(direction.y);
    cam_yaw = atan2f(direction.x, direction.z);

    update_camera();
}

void keyboard(unsigned char key, int x, int y) {
    float camera_speed = 100.0f;
    Position forward;
    Position right;

    (void)x;
    (void)y;

    forward = get_forward_vector();
    right = get_right_vector(forward);

    switch (key) {
        case 'w':
        case 'W':
            cam.lookFrom.x += forward.x * camera_speed;
            cam.lookFrom.y += forward.y * camera_speed;
            cam.lookFrom.z += forward.z * camera_speed;
            update_camera();
            break;
        case 's':
        case 'S':
            cam.lookFrom.x -= forward.x * camera_speed;
            cam.lookFrom.y -= forward.y * camera_speed;
            cam.lookFrom.z -= forward.z * camera_speed;
            update_camera();
            break;
        case 'a':
        case 'A':
            cam.lookFrom.x -= right.x * camera_speed;
            cam.lookFrom.y -= right.y * camera_speed;
            cam.lookFrom.z -= right.z * camera_speed;
            update_camera();
            break;
        case 'd':
        case 'D':
            cam.lookFrom.x += right.x * camera_speed;
            cam.lookFrom.y += right.y * camera_speed;
            cam.lookFrom.z += right.z * camera_speed;
            update_camera();
            break;
        case '+':
            if (time_scale < 1600.0f) {
                time_scale *= 2.0f;
                // printf("Time scale: %f\n", time_scale);
            }
            break;
        case '-':
            if (time_scale > 12.500f) {
                time_scale *= 0.5f;
                // printf("Time scale: %f\n", time_scale);
            }
            break;
        case 27: // ESC
            exit(0);
            break;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    float scroll_speed = 200.0f;
    Position forward;

    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {
            is_dragging = 1;
            last_mouse_x = x;
            last_mouse_y = y;
        } 
        // Scroll move a câmera para frente/trás na direção da visão
        else if (button == 3) { 
            forward = get_forward_vector();
            cam.lookFrom.x += forward.x * scroll_speed;
            cam.lookFrom.y += forward.y * scroll_speed;
            cam.lookFrom.z += forward.z * scroll_speed;
            update_camera();
        } else if (button == 4) { 
            forward = get_forward_vector();
            cam.lookFrom.x -= forward.x * scroll_speed;
            cam.lookFrom.y -= forward.y * scroll_speed;
            cam.lookFrom.z -= forward.z * scroll_speed;
            update_camera();
        }
        glutPostRedisplay();
    } else if (state == GLUT_UP) {
        if (button == GLUT_LEFT_BUTTON) {
            is_dragging = 0;
        }
    }
}

// Chamada quando o mouse se move ENQUANTO o clique está pressionado
void motion(int x, int y) {
    if (is_dragging) {
        // Calcula a diferença entre a posição atual e a anterior
        int dx = x - last_mouse_x;
        int dy = y - last_mouse_y;

        // Atualiza a última posição
        last_mouse_x = x;
        last_mouse_y = y;

        // Sensibilidade do mouse
        float sensitivity = 0.005f; 
        
        // Aplica o delta aos ângulos
        cam_yaw -= dx * sensitivity;
        cam_pitch += dy * sensitivity; 

        update_camera();
        glutPostRedisplay();
    }
}
