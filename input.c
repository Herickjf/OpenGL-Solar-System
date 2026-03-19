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

// Coordenadas esféricas iniciais da câmera
// Os valores foram calculados a partir do seu lookFrom inicial {0, 800, 2500}
static float cam_yaw = 0.0f;
static float cam_pitch = 0.3096f;   // equivalente a asin(800 / sqrt(800^2 + 2500^2))
static float cam_radius = 2624.88f; // equivalente a sqrt(800^2 + 2500^2)

// Recalcula o lookFrom (posição da câmera) baseado no lookAt e nos ângulos
void update_camera() {
    // Limita o pitch para não virar a câmera de cabeça para baixo (Gimbal Lock)
    if (cam_pitch > 1.5f) cam_pitch = 1.5f;   // próximo a +90 graus
    if (cam_pitch < -1.5f) cam_pitch = -1.5f; // próximo a -90 graus

    // Fórmulas de conversão de coordenadas esféricas para cartesianas
    cam.lookFrom.x = cam.lookAt.x + cam_radius * cos(cam_pitch) * sin(cam_yaw);
    cam.lookFrom.y = cam.lookAt.y + cam_radius * sin(cam_pitch);
    cam.lookFrom.z = cam.lookAt.z + cam_radius * cos(cam_pitch) * cos(cam_yaw);
}

void keyboard(unsigned char key, int x, int y) {
    float camera_speed = 100.0f;

    (void)x;
    (void)y;

    switch (key) {
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
        // Scroll agora altera o raio da órbita, fazendo um zoom real em direção ao lookAt
        else if (button == 3) { 
            cam_radius -= scroll_speed;
            if (cam_radius < 100.0f) cam_radius = 100.0f; // Evita atravessar o centro
            update_camera();
        } else if (button == 4) { 
            cam_radius += scroll_speed;
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
