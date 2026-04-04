#include <GL/glut.h>
#include "draw.h"

// funções externas que você já tem
extern void draw_sphere_lod(float radius, float x, float y, float z, float rotation);
extern void draw_stars_background();
extern void draw_rings(Rings *rings, float planet_radius);
extern Position get_position(Body *body);
extern Position get_moon_position(Moon *moon);

// =========================

void drawBackground() {
    draw_stars_background();
}

static void bindTextureSafe(GLuint tex, GLenum unit) {
    if (tex == 0) return;

    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, tex);
}

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

// =========================

void drawSun(Body *sun) {
    float sun_scale = 0.5f;

    apply_material(&sun->material);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sun->texture_id);

    draw_sphere_lod(
        sun->radius * radius_scale * sun_scale,
        0.0f, 0.0f, 0.0f,
        time_sim * (360.0f / sun->rotation_period)
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

// =========================

void drawMoon(Moon *moon) {
    Position pos = get_moon_position(moon);

    glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);

        apply_material(&moon->material);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, moon->texture_id);

        draw_sphere_lod(
            moon->radius * radius_scale,
            pos.x, pos.y, pos.z,
            time_sim * (360.0f / moon->rotation_period)
        );

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// =========================

void drawPlanet(Body *planet) {
    Position pos = get_position(planet);

    glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        glRotatef(planet->axial_tilt, 0.0f, 1.0f, 0.0f);

        apply_material(&planet->material);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, planet->texture_id);
        

        draw_sphere_lod(
            planet->radius * radius_scale,
            pos.x, pos.y, pos.z,
            time_sim * (360.0f / planet->rotation_period)
        );

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        // anéis
        if (planet->rings) {
            glPushMatrix();
                glRotatef(planet->axial_tilt - 180.0f, 1.0f, 0.0f, 0.0f);
                glRotatef(time_sim * 20.0f, 0.0f, 1.0f, 0.0f);
                draw_rings(planet->rings, planet->radius);
            glPopMatrix();
        }

        // luas
        for (int j = 0; j < planet->moons_count; j++) {
            drawMoon(&planet->moons[j]);
        }

    glPopMatrix();
}