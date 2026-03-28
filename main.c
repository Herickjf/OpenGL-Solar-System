// g++ main.c cJSON.c utils.c input.c stb_image.c -o solarSystem -lGL -lGLU -lglut && ./solarSystem

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "calculus.h"
#include "stb_image.h" 
#include "cJSON.h"
#include "bodies.h" 
#include "app_state.h"
#include "input.h"

// controle de fps
int last_frame_time = 0;
int target_fps = 30;  // Limite a 60 FPS
int frame_duration_ms = 1000 / target_fps; 

// scale variables
float distance_scale;
float radius_scale;
float time_scale;

// celestial bodies array
int body_count = 0;
Body *bodies = NULL;

Camera cam = {
    {0.0f, 800.0f,  2500.0f},
    {0.0f, 0.0f,    0.0f},
    {0.0f, 1.0f,    0.0f},
};

// time variables
float time_sim = 0.0f;
int last_time = 0;

GLUquadric *quad;
// logics


void update_timer(int);

void update() {
    int current_time = glutGet(GLUT_ELAPSED_TIME);

    float delta = (current_time - last_time) / 1000.0f; // segundos
    last_time = current_time;

    time_sim += delta * time_scale; // atualiza o tempo aplicando a escala temporal

    // Só redesenha se já passou tempo suficiente desde o último frame
    if (current_time - last_frame_time >= frame_duration_ms) {
        last_frame_time = current_time;
        glutPostRedisplay();
    }

    // Agenda o próximo update após um pequeno delay
    glutTimerFunc(1, update_timer, 0);
}

// Função auxiliar para timer
void update_timer(int value) {
    update();
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

    quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);
    gluQuadricNormals(quad, GLU_SMOOTH);
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
        draw_stars_background();
        Body sun = bodies[0];
        float sun_scale = 0.5f;

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, sun.texture_id);
            draw_sphere_lod(sun.radius * radius_scale * sun_scale, 0.0f, 0.0f, 0.0f, time_sim* (360.0f / sun.rotation_period));
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        // desenha as órbitas primeiro
        for (int i = 0; i < body_count; i++) {
            if (bodies[i].orbit_radius > 0) {  // Ignora o Sol
                draw_orbit(&bodies[i]);
            }
        }

        // Desenhar cada planeta
        for (int i = 1; i < body_count; i++)
        {
            Position planet_pos = get_position(&bodies[i]);
            glPushMatrix(); // Abre a matriz do corpo celeste i             
                glTranslatef(planet_pos.x, planet_pos.y, planet_pos.z); 
                glRotatef(bodies[i].axial_tilt, 0.0f, 1.0f, 0.0f);

                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, bodies[i].texture_id);
                // desenha 
                    draw_sphere_lod(bodies[i].radius * radius_scale, 
                                    planet_pos.x, planet_pos.y, planet_pos.z, 
                                    time_sim * (360.0f / bodies[i].rotation_period));
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D);

                if (bodies[i].rings) {
                    glPushMatrix();
                        // inclinação do plano do anel
                        glRotatef(bodies[i].axial_tilt - 180.0f, 1.0f, 0.0f, 0.0f);

                        // rotação ao redor do planeta (opcional, quase imperceptível)
                        glRotatef(time_sim * 20.0f, 0.0f, 1.0f, 0.0f);
                        draw_rings(bodies[i].rings, bodies[i].radius);
                    glPopMatrix();
                }

                // desenha as luas
                for(int j = 0; j < bodies[i].moons_count; j++){
                    Moon moon = bodies[i].moons[j];
                    Position moon_pos = get_moon_position(&moon);

                    glPushMatrix(); // Abre matriz da lua j
                        glTranslatef(moon_pos.x, moon_pos.y, moon_pos.z);

                        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, moon.texture_id);
                            draw_sphere_lod(moon.radius * radius_scale, 
                                            moon_pos.x, moon_pos.y, moon_pos.z,
                                        time_sim* (360.0f / moon.rotation_period));
                        glBindTexture(GL_TEXTURE_2D, 0);
                        glDisable(GL_TEXTURE_2D);
                    glPopMatrix(); // Fecha matriz da lua j
                }


            glPopMatrix(); // Fecha matriz do corpo celeste i
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

   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(500, 500);
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);

    last_time = glutGet(GLUT_ELAPSED_TIME);
   init();
    init_camera_controller();

   load_all_textures(bodies, body_count);

   // update
   glutTimerFunc(0, update_timer, 0);  // Use timer em vez de idle
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutMainLoop();

   return 0;
}
