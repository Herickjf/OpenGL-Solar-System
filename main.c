// g++ main.c cJSON.c utils.c input.c -o solarSystem -lGL -lGLU -lglut -lm && ./solarSystem

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cJSON.h"
#include "bodies.h"
#include "app_state.h"
#include "input.h"

// scale variables
float distance_scale;
float radius_scale;
float time_scale;

// celestial bodies array
int body_count = 0;
Body *bodies = NULL;

Camera cam = {
    {0.0f, 800.0f, 2500.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
};

// time variables
float time_sim = 0.0f;
int last_time = 0;


// logics
void draw_sphere_lod(float radius, float x, float y, float z) {
    float dx = cam.lookFrom.x - x;
    float dy = cam.lookFrom.y - y;
    float dz = cam.lookFrom.z - z;
    
    float distance = sqrt(dx*dx + dy*dy + dz*dz);

    if (distance < 1.0f) distance = 1.0f;
    int slices = (int)(1000 * radius / distance);
    
    if (slices < 10) slices = 10;
    if (slices > 100) slices = 100;
    
    glutSolidSphere(radius, slices, slices);
}

void update() {
    int current_time = glutGet(GLUT_ELAPSED_TIME);

    float delta = (current_time - last_time) / 1000.0f; // segundos
    last_time = current_time;

    time_sim += delta * time_scale; // atualiza o tempo aplicando a escala temporal

    glutPostRedisplay();
}

Position get_position(Body* body) {
    // Sol fica parado
    if (body->orbit_radius == 0) {
        return (Position){0, 0, 0};
    }

    // velocidade angular
    float angle = time_sim * (2.0f * M_PI / body->orbital_period);

    float a = body->orbit_radius * distance_scale;
    float e = body->eccentricity;

    // distância ao foco (Sol)
    float r = a * (1 - e*e) / (1 + e * cos(angle));

    float x = r * cos(angle);
    float z = r * sin(angle);

    // inclinação da órbita (rotação no eixo X)
    float inc = body->orbit_inclination * M_PI / 180.0f;

    float y_rot = z * sin(inc);
    float z_rot = z * cos(inc);

    return (Position){
        x,
        y_rot,
        z_rot
    };
}

// openGL =====================
void init(void)
{
   GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
   GLfloat mat_shininess[] = {50.0};
   GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};

   glClearColor(0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_SMOOTH);

   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   glLightfv(GL_LIGHT0, GL_POSITION, light_position);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
}

void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

    // Câmera olhando de cima para baixo
    gluLookAt(cam.lookFrom.x, cam.lookFrom.y, cam.lookFrom.z,  
             cam.lookAt.x, cam.lookAt.y, cam.lookAt.z,  
             cam.vUp.x, cam.vUp.y, cam.vUp.z); 

    glPushMatrix(); //sol
        // glTranslatef(0.0f, 0.0f, 0.0f);
        Body sun = bodies[0];
        float sun_scale = 0.5f;
        // glDisable(GL_LIGHTING);
        draw_sphere_lod(sun.radius * radius_scale * sun_scale, 0.0f, 0.0f, 0.0f);
        // glEnable(GL_LIGHTING);

        // Loop percorrendo todos os planetas carregados
        for (int i = 1; i < body_count; i++)
        {
            Position pos = get_position(&bodies[i]);
            glPushMatrix(); // Abre a matriz para este corpo celeste            
                glTranslatef(pos.x, pos.y, pos.z); 
                // TAMANHO: Desenha a esfera usando o raio que veio do JSON em escala
                draw_sphere_lod(bodies[i].radius * radius_scale, pos.x, pos.y, pos.z);
            glPopMatrix(); // Fecha a matriz para o próximo planeta partir do centro novamente
        }
    glPopMatrix(); //sol
   
    glutSwapBuffers();
}

void reshape(int w, int h){
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
   
   // Aumentamos o limite de visão para -15.0 a 15.0 para caber as body_count esferas
//    if (w <= h)
//       glOrtho(-15.0, 15.0, -15.0 * (GLfloat)h / (GLfloat)w,
//               15.0 * (GLfloat)h / (GLfloat)w, -10.0, 10.0);
//    else
//       glOrtho(-15.0 * (GLfloat)w / (GLfloat)h,
//               15.0 * (GLfloat)w / (GLfloat)h, -15.0, 15.0, -10.0, 10.0);
    // glOrtho(-2000, 2000, -2000, 2000, -5000, 5000);
              
    gluPerspective(
        60.0,                 // FOV (campo de visão)
        (float)w / (float)h,  // proporção da tela
        1.0,                  // plano próximo
        20000.0               // plano distante
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char **argv)
{
   bodies = load_bodies("configs.json", &body_count);

   if (body_count == 0)
   {
      printf("Erro: Nenhum planeta lido do JSON");
      exit(1);
   }

   for (int i = 0; i < body_count; i++)
   {
      printf("%s\n", bodies[i].name);
   }

   last_time = glutGet(GLUT_ELAPSED_TIME);

   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(500, 500);
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);
   init();
   glutIdleFunc(update);
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMouseFunc(mouse);
   glutMainLoop();

   return 0;
}
