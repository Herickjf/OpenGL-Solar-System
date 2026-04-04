#include <GL/glut.h>
#include <math.h>
#include <string.h>
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

static float clampf_local(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static Position normalize_position(Position v) {
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length < 1e-6f) {
        return (Position){0.0f, 0.0f, 1.0f};
    }
    return (Position){v.x / length, v.y / length, v.z / length};
}

static Position rotate_y_position(Position v, float degrees) {
    float radians = degrees * (float)M_PI / 180.0f;
    float cosine = cosf(radians);
    float sine = sinf(radians);

    return (Position){
        v.x * cosine + v.z * sine,
        v.y,
        -v.x * sine + v.z * cosine
    };
}

static Position rotate_x_position(Position v, float degrees) {
    float radians = degrees * (float)M_PI / 180.0f;
    float cosine = cosf(radians);
    float sine = sinf(radians);

    return (Position){
        v.x,
        v.y * cosine - v.z * sine,
        v.y * sine + v.z * cosine
    };
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

static void draw_sphere_mesh_with_night_overlay(
    float radius,
    int slices,
    int stacks,
    Position light_dir_local,
    float night_strength
) {
    float theta_step = 2.0f * (float)M_PI / (float)slices;
    float phi_step = (float)M_PI / (float)stacks;

    for (int stack = 0; stack < stacks; stack++) {
        float phi0 = phi_step * (float)stack;
        float phi1 = phi_step * (float)(stack + 1);

        float y0 = cosf(phi0);
        float y1 = cosf(phi1);
        float r0 = sinf(phi0);
        float r1 = sinf(phi1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int slice = 0; slice <= slices; slice++) {
            float theta = theta_step * (float)slice;
            float cos_theta = cosf(theta);
            float sin_theta = sinf(theta);

            Position n0 = {cos_theta * r0, y0, sin_theta * r0};
            Position n1 = {cos_theta * r1, y1, sin_theta * r1};

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

static void draw_earth_night_overlay(Body *planet, Position pos, float body_spin) {
    if (!planet->secondary_texture_id) return;
    if (!planet->name || strcmp(planet->name, "Earth") != 0) return;

    Position world_light = normalize_position((Position){-pos.x, -pos.y, -pos.z});
    Position local_light = rotate_x_position(world_light, 90.0f);
    local_light = rotate_y_position(local_light, -(planet->axial_tilt + body_spin));
    local_light = normalize_position(local_light);

    glPushMatrix();
        glRotatef(body_spin, 0.0f, 1.0f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);

        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, planet->secondary_texture_id);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        draw_sphere_mesh_with_night_overlay(
            planet->radius * radius_scale,
            48,
            24,
            local_light,
            0.9f
        );

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    glPopMatrix();
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
    float body_spin = time_sim * (360.0f / planet->rotation_period);

    glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        glRotatef(planet->axial_tilt, 0.0f, 1.0f, 0.0f);

        apply_material(&planet->material);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, planet->texture_id);
        

        draw_sphere_lod(
            planet->radius * radius_scale,
            pos.x, pos.y, pos.z,
            body_spin
        );

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        draw_earth_night_overlay(planet, pos, body_spin);

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