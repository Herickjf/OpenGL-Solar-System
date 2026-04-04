#include <GL/glut.h>
#include <stdio.h>
#include <string.h>

#include "hud.h"
#include "app_state.h"

// ======================
int show_hud = 1;

#define MAX_UI_ITEMS 256
#define MENU_WIDTH_PCT 0.30f  // 30% da largura da tela
#define PADDING 15
#define ROW_HEIGHT 35
#define BUTTON_MARGIN 5

// Cores (RGBA)
static float COLOR_BG[]      = {0.05f, 0.05f, 0.07f, 0.85f};
static float COLOR_ACCENT[]  = {0.2f, 0.5f, 0.9f, 1.0f};
static float COLOR_BTN[]     = {0.15f, 0.15f, 0.18f, 1.0f};
static float COLOR_TEXT[]    = {1.0f, 1.0f, 1.0f, 1.0f};

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

static void draw_text_left(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

static void draw_modern_button(float x, float y, float w, float h, const char* label, int active) {
    // Fundo do botão
    if (active) glColor4f(0.25f, 0.45f, 0.8f, 1.0f);
    else glColor4fv(COLOR_BTN);
    
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();

    // Borda sutil
    glColor4f(1, 1, 1, 0.2f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();

    // Texto centralizado no botão
    glColor4fv(COLOR_TEXT);
    int txt_w = strlen(label) * 8;
    draw_text_left(x + (w/2) - (txt_w/2), y + (h/2) - 4, label);
}

// ======================
// HUD REFEITA
// ======================
void draw_hud(Body* bodies, int count) {
    if (!show_hud) return;

    screen_w = glutGet(GLUT_WINDOW_WIDTH);
    screen_h = glutGet(GLUT_WINDOW_HEIGHT);
    float menu_w = screen_w * MENU_WIDTH_PCT;
    if (menu_w < 250) menu_w = 250; // Largura mínima

    // Setup 2D
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

    // 1. Fundo do Painel Lateral
    draw_rect(0, 0, menu_w, screen_h, COLOR_BG);
    
    // Borda vertical divisória
    glColor4f(1, 1, 1, 0.1f);
    glBegin(GL_LINES);
        glVertex2f(menu_w, 0);
        glVertex2f(menu_w, screen_h);
    glEnd();

    ui_count = 0;
    float curr_y = screen_h - 40;

    // 2. Cabeçalho de Status
    glColor4fv(COLOR_TEXT);
    char buf[64];
    sprintf(buf, "SYSTEM STATUS | SCALE: %.1fx", time_scale);
    draw_text_left(PADDING, curr_y, buf);
    curr_y -= 40;

    // 3. Lista de Corpos Celestes
    for (int i = 0; i < count; i++) {
        Body* b = &bodies[i];
        
        // Nome do Planeta
        glColor4f(0.7f, 0.8f, 1.0f, 1.0f);
        draw_text_left(PADDING, curr_y + 10, b->name);

        // Botões de Ação do Planeta
        float btn_x = menu_w - 160; 
        float bw = 70;
        float bh = 25;

        // Botão FOCAR
        int is_foc = (focused_body == b);
        draw_modern_button(btn_x, curr_y, bw, bh, "FOCUS", is_foc);
        if (ui_count < MAX_UI_ITEMS) 
            ui_items[ui_count++] = (UIItem){btn_x, curr_y, bw, bh, b, NULL, NULL};

        // Botão SPLINE (Acompanhar)
        draw_modern_button(btn_x + bw + 5, curr_y, bw, bh, "SPLINE", 0); 
        // Note: adicione lógica de estado para 'spline_active' se desejar marcar o botão como selecionado
        
        curr_y -= (ROW_HEIGHT);

        // 4. Renderizar Luas (Sub-item)
        for (int j = 0; j < b->moons_count; j++) {
            Moon* m = &b->moons[j];
            
            glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
            draw_text_left(PADDING + 15, curr_y + 8, "- ");
            draw_text_left(PADDING + 30, curr_y + 8, m->name);

            int is_m_foc = (focused_moon == m);
            draw_modern_button(btn_x, curr_y, bw, bh, "VIEW", is_m_foc);
            if (ui_count < MAX_UI_ITEMS) 
                ui_items[ui_count++] = (UIItem){btn_x, curr_y, bw, bh, NULL, m, b};

            draw_modern_button(btn_x + bw + 5, curr_y, bw, bh, "PATH", 0);

            curr_y -= (ROW_HEIGHT - 5);
        }
        curr_y -= 15; // Espaço entre grupos
    }

    // Restore state
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