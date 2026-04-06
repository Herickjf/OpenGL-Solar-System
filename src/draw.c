/*
 * draw.c — Desenho do Sol, planetas e luas com texturas e materiais; Terra com sobreposição noturna.
 * O fundo de estrelas e as órbitas estão implementados em bodies.c.
 */
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include "draw.h"
#include "bodies.h"

extern void draw_sphere_lod(GLuint tex_id, GLuint normal_tex_id, float radius, float x, float y, float z, float rotation);
extern void draw_stars_background();
extern void draw_rings(Rings *rings, float planet_radius);
extern Position get_position(Body *body);
extern Position get_moon_position(Moon *moon);

/**
*@brief Desenha o plano de fundo do cenário, tipicamente o campo de estrelas.
*@param void
*/
void drawBackground() {
    draw_stars_background();
}

/**
*@brief Restringe um valor de ponto flutuante dentro de um intervalo especificado.
*@param value O valor a ser testado.
*@param min_value O limite inferior.
*@param max_value O limite superior.
*@return O valor limitado entre min_value e max_value.
*/
static float clampf_local(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

/**
*@brief Normaliza um vetor de posição para que ele tenha comprimento unitário.
*@param v A posição (vetor) a ser normalizada.
*@return Um novo vetor de posição com magnitude 1.0.
*/
static Position normalize_position(Position v) {
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length < 1e-6f) return (Position){0.0f, 0.0f, 1.0f};
    return (Position){v.x / length, v.y / length, v.z / length};
}


/** 
*@brief Rotaciona um vetor de posição em torno do eixo X.
*@param v A posição original.
*@param degrees O ângulo de rotação em graus.
*@return A nova posição após a rotação.
*/
static Position rotate_x_position(Position v, float degrees) {
    float radians = degrees * (float)M_PI / 180.0f;
    float cosine = cosf(radians);
    float sine = sinf(radians);
    return (Position){v.x, v.y * cosine - v.z * sine, v.y * sine + v.z * cosine};
}

/**
*@brief Rotaciona um vetor de posição em torno do eixo Y.
*@param v A posição original.
*@param degrees O ângulo de rotação em graus.
*@return A nova posição após a rotação.
*/
static Position rotate_y_position(Position v, float degrees) {
    float radians = degrees * (float)M_PI / 180.0f;
    float cosine = cosf(radians);
    float sine = sinf(radians);
    return (Position){v.x * cosine + v.z * sine, v.y, -v.x * sine + v.z * cosine};
}

/**
*@brief Aplica as propriedades de material (ambiente, difusa, especular, emissiva e brilho) ao contexto OpenGL.
*@param material Ponteiro para a estrutura Material contendo os dados.
*@param void
*/
static void apply_material(const Material* material) {
    GLfloat ambient[4] = {
        material->diffuse[0] * 0.2f,
        material->diffuse[1] * 0.2f,
        material->diffuse[2] * 0.2f,
        material->diffuse[3]
    };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material->emission);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->shininess);
}

/**
*@brief Desenha uma malha esférica aplicando um efeito de mistura para simular luzes noturnas baseadas na direção da luz.
*@param radius Raio da esfera.
*@param slices Número de subdivisões ao longo do eixo Z (meridianos).
*@param stacks Número de subdivisões em torno do eixo Z (paralelos).
*@param light_dir_local Direção da luz em coordenadas locais.
*@param night_strength Intensidade do efeito de textura noturna.
*/
static void draw_sphere_mesh_with_night_overlay(float radius, int slices, int stacks, Position light_dir_local, float night_strength) {
    float theta_step = 2.0f * (float)M_PI / (float)slices;
    float phi_step = (float)M_PI / (float)stacks;
    for (int stack = 0; stack < stacks; stack++) {
        float phi0 = phi_step * (float)stack;
        float phi1 = phi_step * (float)(stack + 1);
        float y0 = cosf(phi0); float y1 = cosf(phi1);
        float r0 = sinf(phi0); float r1 = sinf(phi1);
        glBegin(GL_TRIANGLE_STRIP);
        for (int slice = 0; slice <= slices; slice++) {
            float theta = theta_step * (float)slice;
            float cos_t = cosf(theta); float sin_t = sinf(theta);
            Position n0 = {cos_t * r0, y0, sin_t * r0};
            Position n1 = {cos_t * r1, y1, sin_t * r1};
            float day0 = n0.x * light_dir_local.x + n0.y * light_dir_local.y + n0.z * light_dir_local.z;
            float day1 = n1.x * light_dir_local.x + n1.y * light_dir_local.y + n1.z * light_dir_local.z;
            float night0 = clampf_local(-day0, 0.0f, 1.0f) * night_strength;
            float night1 = clampf_local(-day1, 0.0f, 1.0f) * night_strength;
            glColor4f(1.0f, 1.0f, 1.0f, night0);
            glTexCoord2f((float)slice / (float)slices, (float)stack / (float)stacks);
            glVertex3f(n0.x * radius, n0.y * radius, n0.z * radius);
            glColor4f(1.0f, 1.0f, 1.0f, night1);
            glTexCoord2f((float)slice / (float)slices, (float)(stack + 1) / (float)stacks);
            glVertex3f(n1.x * radius, n1.y * radius, n1.z * radius);
        }
        glEnd();
    }
}

/**
*@brief Gerencia e desenha a sobreposição de texturas noturnas especificamente para o planeta Terra.
*@param planet Ponteiro para o corpo celeste (Terra).
*@param pos Posição atual do planeta no mundo.
*@param body_spin Rotação atual do corpo sobre seu eixo.
*/
static void draw_earth_night_overlay(Body *planet, Position pos, float body_spin) {
    if (!planet->secondary_texture_id) return;
    if (!planet->name || strcmp(planet->name, "Earth") != 0) return;

    Position world_light = normalize_position((Position){-pos.x, -pos.y, -pos.z});
    Position local_light = rotate_x_position(world_light, 90.0f);
    local_light = rotate_y_position(local_light, -(planet->axial_tilt + body_spin));
    local_light = normalize_position(local_light);

    glPushMatrix();
        glRotatef(planet->axial_tilt, 0.0f, 1.0f, 0.0f);
        glRotatef(body_spin, 0.0f, 1.0f, 0.0f);
        glRotatef(-115.0f, 0,1,0);
        glRotatef(-180.0f, 0,0,1);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, planet->secondary_texture_id);
        draw_sphere_mesh_with_night_overlay(planet->radius * radius_scale * 1.01f, 48, 24, local_light, 0.9f);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    glPopMatrix();
}

/**
*@brief Desenha uma camada adicional sobre o planeta para representar o brilho especular (reflexos de água/gelo).
*@param planet Ponteiro para o corpo celeste.
*@param body_spin Rotação atual do corpo sobre seu eixo.
*/
static void draw_specular_overlay(Body *planet, float body_spin) {
    if (!planet->specular_texture_id) return;

    glPushMatrix();
        glRotatef(planet->axial_tilt, 0.0f, 1.0f, 0.0f);
        glRotatef(body_spin, 0.0f, 1.0f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

        float r = planet->radius * radius_scale * 1.002f;

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, planet->specular_texture_id);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDepthMask(GL_FALSE);

        // Usamos a draw_sphere_lod apenas para a geometria, sem aplicar shaders de normal map
        draw_sphere_lod(planet->specular_texture_id, 0, r, 0, 0, 0, 0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

/**
*@brief Renderiza o Sol no centro do sistema, aplicando seu material e rotação.
*@param sun Ponteiro para o corpo celeste representando o Sol.
*/
void drawSun(Body *sun) {
    float sun_scale = 0.5f;
    apply_material(&sun->material);

    // O Sol usa sua textura difusa, mas geralmente não possui normal map (0)
    draw_sphere_lod(
        sun->texture_id, 
        0, 
        sun->radius * radius_scale * sun_scale,
        0.0f, 0.0f, 0.0f,
        time_sim * (360.0f / sun->rotation_period)
    );
}

/**
*@brief Renderiza uma lua em sua posição orbital relativa ao planeta pai.
*@param moon Ponteiro para a estrutura da Lua.
*/
void drawMoon(Moon *moon) {
    Position pos = get_moon_position(moon);

    glPushMatrix();
        // A translação para a posição da lua já é tratada aqui
        glTranslatef(pos.x, pos.y, pos.z);
        apply_material(&moon->material);

        draw_sphere_lod(
            moon->texture_id, 
            moon->normal_texture_id, 
            moon->radius * radius_scale,
            pos.x, pos.y, pos.z,
            time_sim * (360.0f / moon->rotation_period)
        );
    glPopMatrix();
}

/** 
*@brief Renderiza um planeta completo, incluindo sua malha principal, anéis, luas e efeitos de textura (noite/especular).
*@param planet Ponteiro para a estrutura do Planeta.
*/
void drawPlanet(Body *planet) {
    Position pos = get_position(planet);
    float body_spin = time_sim * (360.0f / planet->rotation_period);

    glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        
        apply_material(&planet->material);

        // Chamada principal para o planeta usando a nova assinatura
        draw_sphere_lod(
            planet->texture_id,
            planet->normal_texture_id,
            planet->radius * radius_scale,
            pos.x, pos.y, pos.z,
            body_spin
        );

        // Overlays e efeitos específicos
        draw_earth_night_overlay(planet, pos, body_spin);

        if(planet->specular_texture_id) {
            draw_specular_overlay(planet, body_spin);
        }

        // Desenho dos anéis
        if (planet->rings) {
            glPushMatrix();
                glRotatef(planet->axial_tilt - 180.0f, 1.0f, 0.0f, 0.0f);
                glRotatef(time_sim * 20.0f, 0.0f, 1.0f, 0.0f);
                draw_rings(planet->rings, planet->radius);
            glPopMatrix();
        }

        // Desenho das luas deste planeta
        for (int j = 0; j < planet->moons_count; j++) {
            drawMoon(&planet->moons[j]);
        }

    glPopMatrix();
}