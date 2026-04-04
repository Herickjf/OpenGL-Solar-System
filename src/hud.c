#include <GL/glut.h>
#include <stdio.h>
#include <string.h>

#include "hud.h"
#include "app_state.h"

// ======================
extern int show_hud;
extern CameraMode camera_mode;

#define MAX_UI_ITEMS 256
#define MENU_WIDTH_PCT 0.30f 
#define PADDING 20
#define ROW_HEIGHT 35

// Cores (RGBA)
static float COLOR_BG[]      = {0.05f, 0.05f, 0.07f, 0.90f};
static float COLOR_BTN[]     = {0.12f, 0.12f, 0.15f, 1.0f};
static float COLOR_TEXT[]    = {1.0f, 1.0f, 1.0f, 1.0f};

typedef struct {
    float x, y, w, h;
    Body* body;
    Moon* moon;
    Body* parent;
    CameraMode action; // para diferenciar ações (FOCUS, SPLINE, etc.)
} UIItem;

static UIItem ui_items[MAX_UI_ITEMS];
static int ui_count = 0;
static int screen_w;
static int screen_h;

// ======================
// UTILITÁRIOS DE DESENHO
// ======================

static void draw_rect(float x, float y, float w, float h, float color[4]) {
    glColor4fv(color);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
}

// Função de texto atualizada para aceitar a fonte
static void draw_text(float x, float y, const char* text, void* font) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

static void draw_modern_button(float x, float y, float w, float h, const char* label, int active) {
    if (active) glColor4f(0.2f, 0.5f, 0.9f, 1.0f);
    else glColor4fv(COLOR_BTN);
    
    glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();

    glColor4f(1, 1, 1, 0.3f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();

    glColor4fv(COLOR_TEXT);
    // Para centralizar fontes de tamanho 15 (aprox 9px de largura por char)
    int txt_w = strlen(label) * 9; 
    draw_text(x + (w/2) - (txt_w/2), y + (h/2) - 5, label, GLUT_BITMAP_9_BY_15);
}

// ======================
// HUD ATUALIZADA
// ======================
void draw_hud(Body* bodies, int count) {
    if (!show_hud) return;

    screen_w = glutGet(GLUT_WINDOW_WIDTH);
    screen_h = glutGet(GLUT_WINDOW_HEIGHT);
    float menu_w = screen_w * MENU_WIDTH_PCT;
    if (menu_w < 280) menu_w = 280; 

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screen_w, 0, screen_h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Fundo Lateral
    draw_rect(0, 0, menu_w, screen_h, COLOR_BG);
    
    ui_count = 0;
    float curr_y = screen_h - 40;

    // 2. Título (Tamanho 18)
    glColor4f(0.3f, 0.7f, 1.0f, 1.0f);
    char buf[64];
    sprintf(buf, "SYSTEM STATUS | %.1fx", time_scale);
    draw_text(PADDING, curr_y, buf, GLUT_BITMAP_HELVETICA_18);
    curr_y -= 50;

    // 3. Lista de Corpos
    for (int i = 0; i < count; i++) {
        Body* b = &bodies[i];
        
        // Nome Planeta (Tamanho 15)
        glColor4fv(COLOR_TEXT);
        draw_text(PADDING, curr_y + 8, b->name, GLUT_BITMAP_9_BY_15);

        float btn_x = menu_w - 170; 
        float bw = 75;
        float bh = 26;

        // Botões Planeta
        // Verifica se o corpo é o focado E se o modo de câmera bate com o botão
        int is_foc_active = (focused_body == b && camera_mode == CAMERA_FOLLOW);
        int is_spline_active = (focused_body == b && camera_mode == CAMERA_ORBIT);

        draw_modern_button(btn_x, curr_y, bw, bh, "FOCUS", is_foc_active);
        if (ui_count < MAX_UI_ITEMS) 
            ui_items[ui_count++] = (UIItem){btn_x, curr_y, bw, bh, b, NULL, NULL, CAMERA_FOLLOW};

        draw_modern_button(btn_x + bw + 5, curr_y, bw, bh, "SPLINE", is_spline_active);
        if(ui_count < MAX_UI_ITEMS) 
            ui_items[ui_count++] = (UIItem){btn_x + bw + 5, curr_y, bw, bh, b, NULL, NULL, CAMERA_ORBIT};
        
        curr_y -= ROW_HEIGHT;

        // Luas (Tamanho 15)
        for (int j = 0; j < b->moons_count; j++) {
            Moon* m = &b->moons[j];
            glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
            draw_text(PADDING + 15, curr_y + 8, m->name, GLUT_BITMAP_9_BY_15);

            // Botões Lua
            int is_m_foc_active = (focused_moon == m && camera_mode == CAMERA_FOLLOW);
            int is_m_path_active = (focused_moon == m && camera_mode == CAMERA_ORBIT);

            draw_modern_button(btn_x, curr_y, bw, bh, "FOCUS", is_m_foc_active);
            if (ui_count < MAX_UI_ITEMS) 
                ui_items[ui_count++] = (UIItem){btn_x, curr_y, bw, bh, NULL, m, b, CAMERA_FOLLOW};

            // draw_modern_button(btn_x + bw + 5, curr_y, bw, bh, "PATH", is_m_path_active);
            // if(ui_count < MAX_UI_ITEMS) 
            //     ui_items[ui_count++] = (UIItem){btn_x + bw + 5, curr_y, bw, bh, NULL, m, b, CAMERA_ORBIT};

            curr_y -= ROW_HEIGHT;
        }
        curr_y -= 10;
    }

    // 4. Rodapé com Dicas (Tamanho 15)
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    draw_text(PADDING, 110, "Press 'H' to toggle menu visibility", GLUT_BITMAP_9_BY_15);
    draw_text(PADDING, 80, "Press 'P' to pause simulation", GLUT_BITMAP_9_BY_15);
    draw_text(PADDING, 50, "Press 'R' to pause the music when a body is in focus", GLUT_BITMAP_9_BY_15);
    draw_text(PADDING, 20, "Click on buttons to toggle camera focus/mode", GLUT_BITMAP_9_BY_15);


    glDisable(GL_BLEND);
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

    screen_w = glutGet(GLUT_WINDOW_WIDTH);
    screen_h = glutGet(GLUT_WINDOW_HEIGHT);

    int y = screen_h - mouse_y;

    for (int i = 0; i < ui_count; i++) {

        UIItem* it = &ui_items[i];

        if (mouse_x >= it->x && mouse_x <= it->x + it->w &&
            y >= it->y && y <= it->y + it->h) {

            if (it->body) {
                if(focused_body == it->body && camera_mode == it->action) {
                    focused_body = NULL;
                    cam.lookAt = (Position){0,0,0};
                    cam.lookFrom = (Position){0, 800, 2500};
                    camera_mode = CAMERA_FREE;
                    printf("Foco planeta: nenhum\n");
                } else {
                    camera_zoom = 1.0f;
                    focused_body = it->body;
                    focused_moon = NULL;
                    camera_mode = it->action;
                    printf("Foco planeta: %s\n", it->body->name);
                }
                printf("Foco planeta: %s\n", it->body->name);
            }

            if (it->moon) {
                if(focused_moon == it->moon) {
                    focused_moon = NULL;
                    cam.lookAt = (Position){0,0,0};
                    cam.lookFrom = (Position){0, 800, 2500};
                    camera_mode = CAMERA_FREE;
                    printf("Foco lua: nenhuma\n");
                } else {
                    camera_zoom = 1.0f;
                    focused_moon = it->moon;
                    moon_parent = it->parent;
                    focused_body = NULL;
                    camera_mode = it->action;
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