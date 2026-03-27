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

// Ângulos de visão da câmera (direção para onde a câmera aponta)
// Valores iniciais calculados a partir de lookFrom={0,800,2500} apontando para {0,0,0}
static float cam_yaw   = 0.0f;
static float cam_pitch = -0.3096f; // asin(-800 / sqrt(800^2 + 2500^2))

// Distância fixa usada para posicionar o ponto lookAt à frente de lookFrom
static const float LOOK_DISTANCE = 2624.88f; // sqrt(800^2 + 2500^2)

// Preenche dir[3] com o vetor de direção unitário derivado dos ângulos atuais
static void get_direction(float dir[3]) {
    dir[0] =  cos(cam_pitch) * sin(cam_yaw);
    dir[1] =  sin(cam_pitch);
    dir[2] = -cos(cam_pitch) * cos(cam_yaw);
}

// Recalcula o lookAt (para onde a câmera aponta) com base em lookFrom e nos ângulos
void update_camera() {
    // Limita o pitch para evitar Gimbal Lock
    if (cam_pitch >  1.5f) cam_pitch =  1.5f;
    if (cam_pitch < -1.5f) cam_pitch = -1.5f;

    float dir[3];
    get_direction(dir);

    // lookAt fica a LOOK_DISTANCE à frente de lookFrom na direção calculada
    cam.lookAt.x = cam.lookFrom.x + dir[0] * LOOK_DISTANCE;
    cam.lookAt.y = cam.lookFrom.y + dir[1] * LOOK_DISTANCE;
    cam.lookAt.z = cam.lookFrom.z + dir[2] * LOOK_DISTANCE;
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
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

    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {
            is_dragging = 1;
            last_mouse_x = x;
            last_mouse_y = y;
        }
        // Scroll move a câmera para frente/trás ao longo da direção atual de visão
        else if (button == 3 || button == 4) {
            float dir[3];
            get_direction(dir);
            float sign = (button == 3) ? 1.0f : -1.0f; // 3 = avança, 4 = recua
            cam.lookFrom.x += sign * dir[0] * scroll_speed;
            cam.lookFrom.y += sign * dir[1] * scroll_speed;
            cam.lookFrom.z += sign * dir[2] * scroll_speed;
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

        // Arrasto horizontal gira a câmera para a esquerda/direita (yaw)
        // Arrasto vertical inclina a câmera para cima/baixo (pitch)
        cam_yaw   += dx * sensitivity;
        cam_pitch -= dy * sensitivity; // invertido: arrastar para cima aumenta o pitch

        update_camera();
        glutPostRedisplay();
    }
}
