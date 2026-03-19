#ifndef BODIES_H
#define BODIES_H

#include <GL/glut.h>
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h" 
#include "utils.h"

// =====================
extern float distance_scale;
extern float radius_scale;
extern float time_scale;

// =====================
// Rings (Saturn, etc.)
// =====================
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

    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        printf("Erro ao carregar textura: %s\n", filename);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Parâmetros
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

    // Upload da textura
    gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0); // limpa bind

    return texture;
}

void load_all_textures(Body* bodies, int count) {
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
    moon.orbital_period = get_float(moon_json, "orbital_period");
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
    body.orbital_period = get_float(body_json, "orbital_period");
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

#endif