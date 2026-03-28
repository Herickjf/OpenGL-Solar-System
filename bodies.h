#ifndef BODIES_H
#define BODIES_H

#include <GL/glut.h>
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h" 
#include "utils.h"
#include "calculus.h"
#include "app_state.h"

extern GLUquadric *quad;

// =====================
extern float distance_scale;
extern float radius_scale;
extern float time_scale;

// =====================
// Rings (Saturn, etc.)
typedef struct {
    char* texture_path;
    GLuint texture_id;

    float inner_radius;
    float outer_radius;
} Rings;

// =====================
typedef struct {
    char* name;

    char* texture_path;
    char* normal_texture_path;

    GLuint texture_id;
    GLuint normal_texture_id;

    float radius;
    float orbit_radius;
    float eccentricity;
    float orbit_inclination;
    float axial_tilt;
    float orbital_period;
    float rotation_period;
} Moon;

// =====================
typedef struct Body {
    char* name;
    char* type;

    char* texture_path;
    char* secondary_texture_path;
    char* normal_texture_path;

    GLuint texture_id;
    GLuint secondary_texture_id;
    GLuint normal_texture_id;

    char* orbit_center;

    float orbit_inclination;
    float orbit_radius;
    float eccentricity;
    float orbital_period;

    float radius;
    float axial_tilt;
    float rotation_period;

    struct Body* parent;

    Moon* moons;
    int moons_count;

    Rings* rings;
} Body;

typedef struct {
    char* texture_path;
    GLuint texture_id;
} Stars;

Stars stars;

// HELPERS
// =====================
static char* get_string(cJSON* obj, const char* key) {
    cJSON* item = cJSON_GetObjectItem(obj, key);
    return (item && cJSON_IsString(item)) ? strdup(item->valuestring) : NULL;
}

static float get_float(cJSON* obj, const char* key) {
    cJSON* item = cJSON_GetObjectItem(obj, key);
    return item ? (float)item->valuedouble : 0.0f;
}

static GLuint loadTexture(const char *filename) {
    int width, height, nrChannels;

    stbi_set_flip_vertically_on_load(1);

    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        printf("Erro ao carregar textura: %s\n", filename);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;
    else {
        printf("Formato desconhecido: %d canais\n", nrChannels);
        stbi_image_free(data);
        return 0;
    }

    gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

static void load_all_textures(Body* bodies, int count) {
    if(stars.texture_path){
        stars.texture_id = loadTexture(stars.texture_path);
    }

    for (int i = 0; i < count; i++) {
        Body* b = &bodies[i];

        if (b->texture_path)
            b->texture_id = loadTexture(b->texture_path);

        if (b->secondary_texture_path)
            b->secondary_texture_id = loadTexture(b->secondary_texture_path);

        if (b->normal_texture_path)
            b->normal_texture_id = loadTexture(b->normal_texture_path);

        for (int j = 0; j < b->moons_count; j++) {
            Moon* m = &b->moons[j];

            if (m->texture_path)
                m->texture_id = loadTexture(m->texture_path);

            if (m->normal_texture_path)
                m->normal_texture_id = loadTexture(m->normal_texture_path);
        }

        if (b->rings && b->rings->texture_path) {
            b->rings->texture_id = loadTexture(b->rings->texture_path);
        }
    }
}

// =====================
static Moon parse_moon(cJSON* moon_json) {
    Moon moon;

    moon.name = get_string(moon_json, "name");

    moon.texture_path = get_string(moon_json, "texture");
    moon.normal_texture_path = get_string(moon_json, "normal_texture");

    // moon.texture_id = moon.texture_path ? loadTexture(moon.texture_path) : 0;
    // moon.normal_texture_id = moon.normal_texture_path ? loadTexture(moon.normal_texture_path) : 0;

    moon.radius = get_float(moon_json, "radius");
    moon.orbit_radius = get_float(moon_json, "orbit_radius");
    moon.eccentricity = get_float(moon_json, "eccentricity");
    moon.orbit_inclination = get_float(moon_json, "orbit_inclination");
    moon.axial_tilt = get_float(moon_json, "axial_tilt");
    moon.orbital_period =  - get_float(moon_json, "orbital_period");
    moon.rotation_period = get_float(moon_json, "rotation_period");

    return moon;
}

// =====================
static Rings* parse_rings(cJSON* rings_json) {
    if (!rings_json) return NULL;

    Rings* rings = (Rings*) malloc(sizeof(Rings));

    rings->texture_path = get_string(rings_json, "secondary_texture");
    // rings->texture_id = rings->texture_path ? loadTexture(rings->texture_path) : 0;

    rings->inner_radius = get_float(rings_json, "inner_radius");
    rings->outer_radius = get_float(rings_json, "outer_radius");

    return rings;
}

// =====================
static Body parse_body(cJSON* body_json) {
    Body body;

    body.name = get_string(body_json, "name");
    body.type = get_string(body_json, "type");

    body.texture_path = get_string(body_json, "texture");
    body.secondary_texture_path = get_string(body_json, "secondary_texture");
    body.normal_texture_path = get_string(body_json, "normal_texture");

    // body.texture_id = body.texture_path ? loadTexture(body.texture_path) : 0;
    // body.secondary_texture_id = body.secondary_texture_path ? loadTexture(body.secondary_texture_path) : 0;
    // body.normal_texture_id = body.normal_texture_path ? loadTexture(body.normal_texture_path) : 0;

    body.orbit_center = get_string(body_json, "orbit_center");

    body.radius = get_float(body_json, "radius");
    body.orbit_radius = get_float(body_json, "orbit_radius");
    body.eccentricity = get_float(body_json, "eccentricity");
    body.orbit_inclination = get_float(body_json, "orbit_inclination");
    body.axial_tilt = get_float(body_json, "axial_tilt");
    body.orbital_period = - get_float(body_json, "orbital_period");
    body.rotation_period = get_float(body_json, "rotation_period");

    body.parent = NULL;

    // MOONS
    cJSON* moons_json = cJSON_GetObjectItem(body_json, "moons");

    if (moons_json && cJSON_IsArray(moons_json)) {
        body.moons_count = cJSON_GetArraySize(moons_json);
        body.moons = (Moon*) malloc(sizeof(Moon) * body.moons_count);

        for (int i = 0; i < body.moons_count; i++) {
            body.moons[i] = parse_moon(cJSON_GetArrayItem(moons_json, i));
        }
    } else {
        body.moons = NULL;
        body.moons_count = 0;
    }

    body.rings = parse_rings(cJSON_GetObjectItem(body_json, "rings"));

    return body;
}
// =====================
static Body* find_body_by_name(Body* bodies, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(bodies[i].name, name) == 0) {
            return &bodies[i];
        }
    }
    return NULL;
}

// =====================
static void resolve_hierarchy(Body* bodies, int count) {
    for (int i = 0; i < count; i++) {

        if (!bodies[i].orbit_center) {
            bodies[i].parent = NULL;
            continue;
        }

        Body* parent = find_body_by_name(
            bodies,
            count,
            bodies[i].orbit_center
        );

        if (!parent) {
            printf("Erro: parent %s não encontrado para %s\n",
                   bodies[i].orbit_center,
                   bodies[i].name);
        }

        bodies[i].parent = parent;
    }
}


// ==================
static void load_scale(cJSON* root) {
    cJSON* scale = cJSON_GetObjectItem(root, "scale");

    distance_scale = get_float(scale, "distance_scale");
    radius_scale   = get_float(scale, "radius_scale");
    time_scale     = get_float(scale, "time_scale");
}

// =====================
static Body* load_bodies(const char* path, int* out_count) {
    char* json_data = read_file(path);
    cJSON* root = cJSON_Parse(json_data);

    if(!root){
        printf("Erro: %s\n", cJSON_GetErrorPtr());
        exit(1);
    }

    load_scale(root);

    cJSON* stars_json = cJSON_GetObjectItem(root, "stars");
    if(stars_json)
        stars.texture_path = get_string(stars_json, "texture");


    cJSON* bodies_json = cJSON_GetObjectItem(root, "bodies");

    int count = cJSON_GetArraySize(bodies_json);
    Body* bodies = (Body*) malloc(sizeof(Body) * count);

    for (int i = 0; i < count; i++) {
        bodies[i] = parse_body(cJSON_GetArrayItem(bodies_json, i));
    }

    resolve_hierarchy(bodies, count);

    *out_count = count;
    return bodies;
}

// ==========================
// desenho dos corpos 

static void draw_stars_background() {
    glPushMatrix();

    // segue a câmera (sempre centralizado)
    glTranslatef(cam.lookFrom.x, cam.lookFrom.y, cam.lookFrom.z);

    // desliga coisas que atrapalham
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, stars.texture_id);

    // 🔥 MUITO IMPORTANTE: inverter a esfera
    glScalef(-1.0f, 1.0f, 1.0f);

    // esfera gigante
    gluSphere(quad, 5000.0f, 50, 50);

    // restaura estado
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

static void draw_orbit(Body* body) {
    if (body->orbit_radius == 0) return; // Sol não tem órbita
    
    int segments = 150;  // Quantidade de pontos da curva
    float a = body->orbit_radius * distance_scale;
    float e = body->eccentricity;
    float inc = body->orbit_inclination * M_PI / 180.0f;
    
    glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_LIGHTING);
            glColor4f(1.0f, 1.0f, 1.0f, 0.1f);  // Cor cinza
            
            glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= segments; i++) {
                    float angle = 2.0f * M_PI * i / segments;
                    
                    // Equação da órbita (elipse)
                    float r = a * (1 - e*e) / (1 + e * cos(angle));
                    
                    float x = r * cos(angle);
                    float z = r * sin(angle);
                    
                    // Aplica inclinação
                    float y = z * sin(inc);
                    float z_rot = z * cos(inc);
                    
                    glVertex3f(x, y, z_rot);
                }
                
            glEnd();
        glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void draw_sphere_lod(float radius, float x, float y, float z, float body_spin) {
    float dx = cam.lookFrom.x - x;
    float dy = cam.lookFrom.y - y;
    float dz = cam.lookFrom.z - z;
    
    float distance = sqrt(dx*dx + dy*dy + dz*dz);

    if (distance < 1.0f) distance = 1.0f;

    int slices = (int)(1000 * radius / distance);
    
    if (slices < 10) slices = 10;
    if (slices > 100) slices = 100;

    // encapsula a rotação da esfera
    glPushMatrix();
        glRotatef(body_spin, 0.0f, 1.0f, 0.0f); // rotação do corpo de fato
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // correção de eixo da gluSphere
        gluSphere(quad, radius, slices, slices);
    glPopMatrix();
}

static void draw_rings(Rings* rings, float planet_radius) {
    if (!rings) return;

    float base = planet_radius * radius_scale;

    float inner = base * 1.2f;
    float outer = base * 2.0f;

    int segments = 100;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, rings->texture_id);

    // essencial pra transparência da textura dos anéis
    glDisable(GL_BLEND);

    // evita problema de iluminação estranha
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;

        float cosA = cos(angle);
        float sinA = sin(angle);

        // ---- coordenadas ----
        float x_inner = inner * cosA;
        float z_inner = inner * sinA;

        float x_outer = outer * cosA;
        float z_outer = outer * sinA;

        // ---- UV (radial) ----
        float u = (float)i / segments;

        float u_min = 0.2f;
        float u_max = 0.8f;

        // interno
        glTexCoord2f(u_min, u);
        glVertex3f(x_inner, 0.0f, z_inner);
        // externo
        glTexCoord2f(u_max, u);
        glVertex3f(x_outer, 0.0f, z_outer);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}


#endif