#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "app_state.h"
#include "input.h"
#include "structures.h"
#include "hud.h"

// Variáveis de estado do mouse
static int is_dragging = 0;
static int last_mouse_x = 0;
static int last_mouse_y = 0;

// Ângulos da orientação da câmera (free look)
static float cam_yaw = 0.0f;
static float cam_pitch = 0.0f;

// Velocidades ajustáveis
static float camera_speed = 50.0f;
static float scroll_speed = 100.0f;
static float mouse_sensitivity = 0.003f;  // Aumentada para resposta mais rápida
static float look_distance = 1000.0f;
static float previous_time_scale = 32.0f;
int is_paused = 0;


// Opção: Inverter eixos do mouse (descomente se preferir)
// #define INVERT_MOUSE_X
// #define INVERT_MOUSE_Y

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

static Position get_up_vector(void) {
    return normalize(cam.vUp);
}

// Recalcula o lookAt com base na posição atual e na direção do olhar
void update_camera() {
    const float pitch_limit = 89.9f * (float)M_PI / 180.0f;
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
    Position forward, right, up;
    float current_speed = camera_speed;

    int mod = glutGetModifiers();
    if (mod == GLUT_ACTIVE_SHIFT) {
        current_speed *= 3.0f;
    }
    else if (mod == GLUT_ACTIVE_CTRL) {
        current_speed *= 0.3f;
    }

    forward = get_forward_vector();
    right = get_right_vector(forward);
    up = get_up_vector();

    (void)x;
    (void)y;

    switch (key) {
        case 'w':
        case 'W':
            cam.lookFrom.x += forward.x * current_speed;
            cam.lookFrom.y += forward.y * current_speed;
            cam.lookFrom.z += forward.z * current_speed;
            update_camera();
            break;
        case 's':
        case 'S':
            cam.lookFrom.x -= forward.x * current_speed;
            cam.lookFrom.y -= forward.y * current_speed;
            cam.lookFrom.z -= forward.z * current_speed;
            update_camera();
            break;
        case 'a':
        case 'A':
            cam.lookFrom.x -= right.x * current_speed;
            cam.lookFrom.y -= right.y * current_speed;
            cam.lookFrom.z -= right.z * current_speed;
            update_camera();
            break;
        case 'd':
        case 'D':
            cam.lookFrom.x += right.x * current_speed;
            cam.lookFrom.y += right.y * current_speed;
            cam.lookFrom.z += right.z * current_speed;
            update_camera();
            break;
        case 'e':
        case 'E':
            cam.lookFrom.x += up.x * current_speed;
            cam.lookFrom.y += up.y * current_speed;
            cam.lookFrom.z += up.z * current_speed;
            update_camera();
            break;
        case 'q':
        case 'Q':
            cam.lookFrom.x -= up.x * current_speed;
            cam.lookFrom.y -= up.y * current_speed;
            cam.lookFrom.z -= up.z * current_speed;
            update_camera();
            break;
        case ' ':
            cam.lookFrom.x += up.x * current_speed;
            cam.lookFrom.y += up.y * current_speed;
            cam.lookFrom.z += up.z * current_speed;
            update_camera();
            break;
        case '+':
        case '=':
            if (time_scale < 1024.0f) { // multiplo de 2
                time_scale *= 2.0f;
                printf("Time scale: %.2fx\n", time_scale);
            }
            break;
        case '-':
            if (time_scale > 0) { // Evita escala negativa
                time_scale *= 0.5f;
                printf("Time scale: %.2fx\n", time_scale);
            }
            break;
        case 'r':
        case 'R':
            time_scale = 100.0f;
            printf("Time scale reset to: %.2fx\n", time_scale);
            break;
        case 'h':
        case 'H':
            show_hud = !show_hud;
            break;

        case 'p':
        case 'P':
            is_paused = !is_paused;
            if(is_paused) {
                previous_time_scale = time_scale;
                time_scale = 0.0f;
                printf("Simulation paused\n");
            } else {
                time_scale = previous_time_scale;
                printf("Simulation resumed, time scale: %.2fx\n", time_scale);
            }
            break;
        case 27:
            exit(0);
            break;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    Position forward, up, right;
    float current_scroll_speed = scroll_speed;

    int mod = glutGetModifiers();
    if (mod == GLUT_ACTIVE_SHIFT) {
        current_scroll_speed *= 3.0f;
    }

    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {
            is_dragging = 1;
            last_mouse_x = x;
            last_mouse_y = y;
            glutSetCursor(GLUT_CURSOR_NONE);
            hud_click(x, y);
        } 
        else if (button == 3) {
            forward = get_forward_vector();
            cam.lookFrom.x += forward.x * current_scroll_speed;
            cam.lookFrom.y += forward.y * current_scroll_speed;
            cam.lookFrom.z += forward.z * current_scroll_speed;
            update_camera();
        } else if (button == 4) {
            forward = get_forward_vector();
            cam.lookFrom.x -= forward.x * current_scroll_speed;
            cam.lookFrom.y -= forward.y * current_scroll_speed;
            cam.lookFrom.z -= forward.z * current_scroll_speed;
            update_camera();
        }
        glutPostRedisplay();
    } else if (state == GLUT_UP) {
        if (button == GLUT_LEFT_BUTTON) {
            is_dragging = 0;
            glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
        }
    }
}

// ============================================
// MOVIMENTAÇÃO MELHORADA DO MOUSE
// ============================================
void motion(int x, int y) {
    if (is_dragging) {
        int dx = x - last_mouse_x;
        int dy = y - last_mouse_y;

        last_mouse_x = x;
        last_mouse_y = y;

        // ESTILO 1: "PUXAR" a cena (arrastar para esquerda = olhar para direita)
        // Esse é o mais intuitivo para modelagem 3D e visualização
        cam_yaw += dx * mouse_sensitivity;      // Mouse esquerda (+) → Olha direita
        cam_pitch += dy * mouse_sensitivity;    // Mouse cima (+) → Olha baixo
        
        // Se quiser inverter o eixo Y (mouse cima = olha cima), use:
        // cam_pitch -= dy * mouse_sensitivity;
        
        // Limita pitch para não virar de ponta-cabeça
        const float max_pitch = 89.9f * (float)M_PI / 180.0f;
        if (cam_pitch > max_pitch) cam_pitch = max_pitch;
        if (cam_pitch < -max_pitch) cam_pitch = -max_pitch;

        update_camera();
        glutPostRedisplay();
    }
}