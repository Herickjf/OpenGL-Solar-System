#include "bodies.h"
#include "../libs/stb_image.h"
#include "utils.h"
#include "calculus.h"
#include "app_state.h"

Stars stars; 
GLfloat scene_ambient[4] = {0.05f, 0.05f, 0.08f, 1.0f};
GLfloat scene_shininess = 32.0f;

// =====================
// helpers
char* get_string(cJSON* obj, const char* key) {
    cJSON* item = cJSON_GetObjectItem(obj, key);
    return (item && cJSON_IsString(item)) ? strdup(item->valuestring) : NULL;
}

float get_float(cJSON* obj, const char* key) {
    cJSON* item = cJSON_GetObjectItem(obj, key);
    return item ? (float)item->valuedouble : 0.0f;
}

static void set_default_material(Material* material) {
    material->diffuse[0] = 1.0f;
    material->diffuse[1] = 1.0f;
    material->diffuse[2] = 1.0f;
    material->diffuse[3] = 1.0f;

    material->specular[0] = 0.0f;
    material->specular[1] = 0.0f;
    material->specular[2] = 0.0f;
    material->specular[3] = 1.0f;

    material->emission[0] = 0.0f;
    material->emission[1] = 0.0f;
    material->emission[2] = 0.0f;
    material->emission[3] = 1.0f;

    material->shininess = scene_shininess;
}

static void read_rgba_array(cJSON* obj, const char* key, GLfloat out[4]) {
    cJSON* array = cJSON_GetObjectItem(obj, key);

    if (!array || !cJSON_IsArray(array)) {
        return;
    }

    int size = cJSON_GetArraySize(array);
    for (int i = 0; i < 4; i++) {
        cJSON* component = (i < size) ? cJSON_GetArrayItem(array, i) : NULL;
        if (component) {
            out[i] = (GLfloat)component->valuedouble;
        }
    }
}

static Material parse_material(cJSON* material_json, const char* type, const char* name) {
    Material material;
    int has_emission = 0;
    set_default_material(&material);

    if (material_json && cJSON_IsObject(material_json)) {
        read_rgba_array(material_json, "diffuse", material.diffuse);
        read_rgba_array(material_json, "specular", material.specular);
        if (cJSON_GetObjectItem(material_json, "emission")) {
            read_rgba_array(material_json, "emission", material.emission);
            has_emission = 1;
        }

        cJSON* shininess = cJSON_GetObjectItem(material_json, "shininess");
        if (shininess) {
            material.shininess = (GLfloat)shininess->valuedouble;
        }
    }

    if (!has_emission && ((type && strcmp(type, "star") == 0) || (name && strcmp(name, "Sun") == 0))) {
        for (int i = 0; i < 4; i++) {
            material.emission[i] = material.diffuse[i];
        }
    }

    return material;
}

// =====================
// texture
GLuint loadTexture(const char *filename) {
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
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;
    else {
        printf("Formato desconhecido: %d\n", nrChannels);
        stbi_image_free(data);
        return 0;
    }

    gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void load_all_textures(Body* bodies, int count) {
    if(stars.texture_path)
        stars.texture_id = loadTexture(stars.texture_path);

    for (int i = 0; i < count; i++) {
        Body* b = &bodies[i];

        if (b->texture_path)
            b->texture_id = loadTexture(b->texture_path);

        if (b->secondary_texture_path)
            b->secondary_texture_id = loadTexture(b->secondary_texture_path);

        if (b->normal_texture_path)
            b->normal_texture_id = loadTexture(b->normal_texture_path);

        if (b->specular_texture_path)
            b->specular_texture_id = loadTexture(b->specular_texture_path);

        for (int j = 0; j < b->moons_count; j++) {
            Moon* m = &b->moons[j];

            if (m->texture_path)
                m->texture_id = loadTexture(m->texture_path);

            if (m->normal_texture_path)
                m->normal_texture_id = loadTexture(m->normal_texture_path);
        }

        if (b->rings && b->rings->texture_path)
            b->rings->texture_id = loadTexture(b->rings->texture_path);
    }
}

// =====================
// parsing
Moon parse_moon(cJSON* moon_json) {
    Moon moon;

    moon.name = get_string(moon_json, "name");
    moon.texture_path = get_string(moon_json, "texture");
    moon.normal_texture_path = get_string(moon_json, "normal_texture");
    moon.secondary_texture_path = get_string(moon_json, "secondary_texture");
    moon.material = parse_material(cJSON_GetObjectItem(moon_json, "material"), NULL, moon.name);

    moon.radius = get_float(moon_json, "radius");
    moon.orbit_radius = get_float(moon_json, "orbit_radius");
    moon.eccentricity = get_float(moon_json, "eccentricity");
    moon.orbit_inclination = get_float(moon_json, "orbit_inclination");
    moon.axial_tilt = get_float(moon_json, "axial_tilt");
    moon.orbital_period = -get_float(moon_json, "orbital_period");
    moon.rotation_period = get_float(moon_json, "rotation_period");

    return moon;
}

Rings* parse_rings(cJSON* rings_json) {
    if (!rings_json) return NULL;

    Rings* rings = (Rings *) malloc(sizeof(Rings));

    rings->texture_path = get_string(rings_json, "secondary_texture");
    rings->inner_radius = get_float(rings_json, "inner_radius");
    rings->outer_radius = get_float(rings_json, "outer_radius");

    return rings;
}

Body parse_body(cJSON* body_json) {
    Body body;

    body.name = get_string(body_json, "name");
    body.type = get_string(body_json, "type");

    body.texture_path = get_string(body_json, "texture");
    body.secondary_texture_path = get_string(body_json, "secondary_texture");
    body.normal_texture_path = get_string(body_json, "normal_texture");
    body.material = parse_material(cJSON_GetObjectItem(body_json, "material"), body.type, body.name);

    body.orbit_center = get_string(body_json, "orbit_center");

    body.radius = get_float(body_json, "radius");
    body.orbit_radius = get_float(body_json, "orbit_radius");
    body.eccentricity = get_float(body_json, "eccentricity");
    body.orbit_inclination = get_float(body_json, "orbit_inclination");
    body.axial_tilt = get_float(body_json, "axial_tilt");
    body.orbital_period = -get_float(body_json, "orbital_period");
    body.rotation_period = get_float(body_json, "rotation_period");

    body.parent = NULL;

    // moons
    cJSON* moons_json = cJSON_GetObjectItem(body_json, "moons");

    if (moons_json && cJSON_IsArray(moons_json)) {
        body.moons_count = cJSON_GetArraySize(moons_json);
        body.moons = (Moon *) malloc(sizeof(Moon) * body.moons_count);

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
// utils
Body* find_body_by_name(Body* bodies, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(bodies[i].name, name) == 0)
            return &bodies[i];
    }
    return NULL;
}

void resolve_hierarchy(Body* bodies, int count) {
    for (int i = 0; i < count; i++) {

        if (!bodies[i].orbit_center) {
            bodies[i].parent = NULL;
            continue;
        }

        Body* parent = find_body_by_name(bodies, count, bodies[i].orbit_center);

        if (!parent) {
            printf("Erro: parent %s não encontrado para %s\n",
                   bodies[i].orbit_center,
                   bodies[i].name);
        }

        bodies[i].parent = parent;
    }
}

void load_scale(cJSON* root) {
    cJSON* scale = cJSON_GetObjectItem(root, "scale");
    cJSON* lighting = cJSON_GetObjectItem(root, "lighting");

    distance_scale = get_float(scale, "distance_scale");
    radius_scale   = get_float(scale, "radius_scale");
    time_scale     = get_float(scale, "time_scale");

    if (lighting && cJSON_IsObject(lighting)) {
        read_rgba_array(lighting, "ambient", scene_ambient);

        cJSON* shininess = cJSON_GetObjectItem(lighting, "shininess");
        if (shininess) {
            scene_shininess = (GLfloat)shininess->valuedouble;
        }
    }
}

Body* load_bodies(const char* path, int* out_count) {
    char* json_data = read_file(path);
    cJSON* root = cJSON_Parse(json_data);

    if (!root) {
        printf("Erro: %s\n", cJSON_GetErrorPtr());
        exit(1);
    }

    load_scale(root);

    cJSON* stars_json = cJSON_GetObjectItem(root, "stars");
    if(stars_json)
        stars.texture_path = get_string(stars_json, "texture");

    cJSON* bodies_json = cJSON_GetObjectItem(root, "bodies");

    int count = cJSON_GetArraySize(bodies_json);
    Body* bodies = (Body *) malloc(sizeof(Body) * count);

    for (int i = 0; i < count; i++) {
        bodies[i] = parse_body(cJSON_GetArrayItem(bodies_json, i));
    }

    resolve_hierarchy(bodies, count);

    *out_count = count;
    return bodies;
}

// =====================
// draw

void draw_stars_background() {
    glPushMatrix();

    glTranslatef(cam.lookFrom.x, cam.lookFrom.y, cam.lookFrom.z);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, stars.texture_id);

    glScalef(-1.0f, 1.0f, 1.0f);
    gluSphere(quad, 5000.0f, 50, 50);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void draw_orbit(Body* body) {
    if (body->orbit_radius == 0) return;

    int segments = 150;
    float a = body->orbit_radius * distance_scale;
    float e = body->eccentricity;
    float inc = body->orbit_inclination * M_PI / 180.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_LIGHTING);
    glColor4f(1,1,1,0.15f);

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;

        float r = a * (1 - e*e) / (1 + e * cos(angle));

        float x = r * cos(angle);
        float z = r * sin(angle);

        float y = z * sin(inc);
        float zr = z * cos(inc);

        glVertex3f(x, y, zr);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void draw_sphere_lod(float radius, float x, float y, float z, float body_spin) {
    float dx = cam.lookFrom.x - x;
    float dy = cam.lookFrom.y - y;
    float dz = cam.lookFrom.z - z;

    float distance = sqrt(dx*dx + dy*dy + dz*dz);
    if (distance < 1.0f) distance = 1.0f;

    int slices = (int)(1000 * radius / distance);

    if (slices < 10) slices = 10;
    if (slices > 100) slices = 100;

    glPushMatrix();
    glRotatef(body_spin, 0,1,0);
    glRotatef(-90, 1,0,0);

    gluSphere(quad, radius, slices, slices);

    glPopMatrix();
}

void draw_rings(Rings* rings, float planet_radius) {
    if (!rings) return;

    float base = planet_radius * radius_scale;
    float inner = base * 1.2f;
    float outer = base * 2.0f;

    int segments = 100;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, rings->texture_id);

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float angle = 2 * M_PI * i / segments;

        float cosA = cos(angle);
        float sinA = sin(angle);

        float x1 = inner * cosA;
        float z1 = inner * sinA;

        float x2 = outer * cosA;
        float z2 = outer * sinA;

        float u = (float)i / segments;

        glTexCoord2f(0.2f, u);
        glVertex3f(x1, 0, z1);

        glTexCoord2f(0.8f, u);
        glVertex3f(x2, 0, z2);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}