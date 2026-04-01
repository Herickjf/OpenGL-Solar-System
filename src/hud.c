#include <GL/glut.h>
#include <stdio.h>
#include <string.h>

#include "hud.h"
#include "app_state.h"

// ======================
int show_hud = 1;

#define MAX_UI_ITEMS 256

typedef struct {
    float x, y, w, h;
    Body* body;
    Moon* moon;
    Body* parent;
} UIItem;

static UIItem ui_items[MAX_UI_ITEMS];
static int ui_count = 0;

static int screen_w;
static int screen_h;

// ======================
// TEXTO CENTRALIZADO
// ======================
static void draw_text_center(float cx, float cy, const char* text) {
    int width = strlen(text) * 8; // approx GLUT_BITMAP_8_BY_13
    glRasterPos2f(cx - width / 2, cy);

    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    }
}

// ======================
// BOTÃO
// ======================
static void draw_button(float x, float y, float w, float h, const char* label, int selected) {

    if (selected)
        glColor3f(0.3f, 0.6f, 1.0f); // azul
    else
        glColor3f(0.2f, 0.2f, 0.2f); // cinza

    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();

    // borda
    glColor3f(1,1,1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();

    // texto
    glColor3f(1,1,1);
    draw_text_center(x + w/2, y + h/2 - 4, label);
}

// ======================
// HUD
// ======================
void draw_hud(Body* bodies, int count) {
    if (!show_hud) return;

    screen_w = glutGet(GLUT_WINDOW_WIDTH);
    screen_h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screen_w, 0, screen_h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    ui_count = 0;

    // ======================
    // INFO SUPERIOR
    // ======================
    char buffer[128];

    sprintf(buffer, "Press 'H' to toggle HUD, 'P' to pause/resume");
    draw_text_center(screen_w / 2, screen_h - 20, buffer);

    sprintf(buffer, "Time: %.2fx", time_scale);
    draw_text_center(screen_w / 2, screen_h - 40, buffer);

    sprintf(buffer, "Cam: (%.0f %.0f %.0f)",
            cam.lookFrom.x,
            cam.lookFrom.y,
            cam.lookFrom.z);
    draw_text_center(screen_w / 2, screen_h - 60, buffer);

    // ======================
    // GRID DE BOTÕES
    // ======================
    float start_x;

    if(screen_w < 1000) {
        start_x = 80;
    }else{
        start_x = (screen_w / 2) - (5*10 + 6.5*120);
    }
    float start_y = screen_h - 120;

    float btn_w = 120;
    float btn_h = 40;

    float gap_x = 10;
    float gap_y = 10;

    int cols = (screen_w - 40) / (btn_w + gap_x);
    if (cols < 1) cols = 1;

    int col = 0;
    int row = 0;

    // -------- PLANETAS + LUAS --------
    for (int i = 0; i < count; i++) {

        Body* b = &bodies[i];

        float x = start_x + col * (btn_w + gap_x);
        float y = start_y - row * (btn_h + gap_y);

        int selected = (focused_body == b);

        draw_button(x, y, btn_w, btn_h, b->name, selected);

        if (ui_count < MAX_UI_ITEMS) {
            ui_items[ui_count++] = (UIItem){x,y,btn_w,btn_h,b,NULL,NULL};
        }

        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }

        // -------- LUAS --------
        for (int j = 0; j < b->moons_count; j++) {

            Moon* m = &b->moons[j];

            x = start_x + col * (btn_w + gap_x);
            y = start_y - row * (btn_h + gap_y);

            selected = (focused_moon == m);

            draw_button(x, y, btn_w, btn_h, m->name, selected);

            if (ui_count < MAX_UI_ITEMS) {
                ui_items[ui_count++] = (UIItem){x,y,btn_w,btn_h,NULL,m,b};
            }

            col++;
            if (col >= cols) {
                col = 0;
                row++;
            }
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

// ======================
// CLICK
// ======================
void hud_click(int mouse_x, int mouse_y) {
    if (!show_hud) return;

    int y = screen_h - mouse_y;

    for (int i = 0; i < ui_count; i++) {

        UIItem* it = &ui_items[i];

        if (mouse_x >= it->x && mouse_x <= it->x + it->w &&
            y >= it->y && y <= it->y + it->h) {

            if (it->body) {
                if(focused_body == it->body) {
                    focused_body = NULL;
                    cam.lookAt = (Position){0,0,0};
                    cam.lookFrom = (Position){0, 800, 2500};
                    printf("Foco planeta: nenhum\n");
                } else {
                    camera_zoom = 1.0f;
                    focused_body = it->body;
                    focused_moon = NULL;
                    printf("Foco planeta: %s\n", it->body->name);
                }
                printf("Foco planeta: %s\n", it->body->name);
            }

            if (it->moon) {
                if(focused_moon == it->moon) {
                    focused_moon = NULL;
                    cam.lookAt = (Position){0,0,0};
                    cam.lookFrom = (Position){0, 800, 2500};
                    printf("Foco lua: nenhuma\n");
                } else {
                    camera_zoom = 1.0f;
                    focused_moon = it->moon;
                    moon_parent = it->parent;
                    focused_body = NULL;
                    printf("Foco lua: %s\n", it->moon->name);
                }
                printf("Foco lua: %s\n", it->moon->name);
            }

            return;
        }
    }
}

// ======================
void toggle_hud() {
    show_hud = !show_hud;
}

void init_hud() {
    show_hud = 1;
}