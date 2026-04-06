/*
 * Compilação (exemplo Linux — executar na raiz do projeto):
 * g++ main.c src/bodies.c src/hud.c src/audio_controller.c src/camera_controller.c \
 *   libs/cJSON.c src/utils.c src/calculus.c src/input.c src/draw.c src/stb_image.c \
 *   -Iinclude -o solarSystem -lGL -lGLU -lglut -lGLEW -lSDL2 -lSDL2_mixer && ./solarSystem
 */

/*
 * main.c — ponto de entrada e laço principal da aplicação.
 *
 * Responsabilidades:
 *   - Carregar a cena a partir de configs.json e texturas.
 *   - Criar a janela com GLUT, inicializar GLEW e o estado OpenGL (luz, profundidade, quadric para esferas).
 *   - Registrar temporizador e callbacks (teclado, mouse, redesenho).
 *   - A cada tick: avançar o tempo simulado, atualizar câmera (modos seguir/órbita) e áudio (SDL).
 *   - Desenhar a cena (fundo, Sol, órbitas, planetas) e o HUD por cima.
 *
 * Variáveis globais abaixo são compartilhadas com outros módulos via app_state.h / definições externas.
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

/* --- Taxa de redesenho alvo (não confundir com time_scale da simulação) --- */
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

/*
 * Callback do temporizador GLUT: coração da simulação.
 * - delta: segundos desde o último tick (relógio real).
 * - time_sim acumula o tempo da animação multiplicado por time_scale (pausa = 0, acelerar com +/−).
 * - update_camera(delta): apenas afeta modos FOLLOW/ORBIT (camera_controller.c).
 * - update_audio(): troca música conforme foco no HUD (SDL2_mixer).
 * - glutPostRedisplay é limitado por frame_duration_ms para não desenhar em excesso.
 */
void update_timer(int value) {
    int current_time = glutGet(GLUT_ELAPSED_TIME);
    float delta = (current_time - last_time) / 1000.0f;
    last_time = current_time;

    time_sim += delta * time_scale;

    update_camera(delta);
    update_audio();

    if (current_time - last_frame_time >= frame_duration_ms) {
        last_frame_time = current_time;
        glutPostRedisplay();
    }
    glutTimerFunc(1, update_timer, 0);
}

/* Estado OpenGL “clássico”: suavização, material padrão, uma luz, teste de profundidade, quadric texturizado para esferas. */
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

/*
 * Um frame completo:
 * 1) Limpa buffers e posiciona a câmera com gluLookAt a partir de cam (globais).
 * 2) Luz pontual na origem (Sol no centro da cena).
 * 3) Desenha fundo, Sol, órbitas de todos os planetas, depois planetas (índice 1..n — 0 é o Sol).
 * 4) HUD em coordenadas de ecrã (desenhado dentro de draw_hud).
 */
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

/* Atualiza viewport e matriz de projeção perspetiva quando a janela muda de tamanho. */
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w / (float)h, 1.0, 20000.0);
    glMatrixMode(GL_MODELVIEW);
}

/*
 * Sequência de arranque:
 * 1) Carga JSON e contagem de corpos (falha se vazio).
 * 2) GLUT: janela 800×600, double buffer, Z-buffer.
 * 3) GLEW após contexto criado.
 * 4) init OpenGL, HUD, orientação inicial da câmara (yaw/pitch), SDL áudio, texturas em GPU.
 * 5) Registo de timer + input + display e entrada no laço bloqueante glutMainLoop.
 */
int main(int argc, char **argv) {
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Solar System");
    
    glewInit();
    last_time = glutGet(GLUT_ELAPSED_TIME);
    bodies = load_bodies("configs.json", &body_count);
    if (body_count == 0) exit(1);

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