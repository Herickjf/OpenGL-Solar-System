/*
 * bodies.c — Leitura de configs.json (cJSON), texturas via stb_image e geometria auxiliar da cena:
 * fundo de estrelas, traço das órbitas, esfera com LOD e anéis planetários.
 *
 * Adicionado suporte a bump mapping (normal mapping) via Shader Program para planetas compatíveis.
 */

#include <GL/glew.h>
#include <GL/glut.h>
#include "bodies.h"
#include "../libs/stb_image.h"
#include "utils.h"
#include "calculus.h"
#include "app_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define BUMP_ACTIVE 1

Stars stars; 
GLfloat scene_ambient[4] = {0.05f, 0.05f, 0.08f, 1.0f};
GLfloat scene_shininess = 32.0f;

// ID global para o shader program que gerencia normal mapping nas esferas
static GLuint sphere_shader_program = 0;

// ==========================================
// Fontes dos Shaders de Normal Mapping (Bump Mapping)
// ==========================================
const char* sphere_vertex_shader_source =
    "varying vec2 vTexCoord;\n"
    "varying vec3 vLightDir;\n"
    "varying vec3 vViewDir;\n"
    "varying vec3 vNormal;\n"
    "void main() {\n"
    "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "    vTexCoord = gl_MultiTexCoord0.xy;\n"
    "    vec4 viewSpacePos = gl_ModelViewMatrix * gl_Vertex;\n"
    "    vLightDir = normalize(gl_LightSource[0].position.xyz - viewSpacePos.xyz);\n"
    "    vViewDir = normalize(-viewSpacePos.xyz);\n"
    "    vNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
    "}";

const char* sphere_fragment_shader_source =
    "varying vec2 vTexCoord;\n"
    "varying vec3 vLightDir;\n"
    "varying vec3 vViewDir;\n"
    "varying vec3 vNormal;\n"
    "uniform sampler2D uDiffuseTexture;\n"
    "uniform sampler2D uNormalMap;\n"
    "uniform bool uHasNormalMap;\n"
    "void main() {\n"
    "    vec3 finalNormal = vNormal;\n"
    "    if (uHasNormalMap) {\n"
    "        vec3 textureNormal = texture2D(uNormalMap, vTexCoord).xyz * 2.0 - 1.0;\n"
    "        finalNormal = normalize(vNormal + textureNormal * 0.5);\n"
    "    }\n"
    "    vec4 diffuseColor = texture2D(uDiffuseTexture, vTexCoord);\n"
    "    float lambertian = max(dot(finalNormal, vLightDir), 0.0);\n"
    "    vec3 diffuse = lambertian * diffuseColor.rgb * gl_FrontMaterial.diffuse.rgb * gl_LightSource[0].diffuse.rgb;\n"
    "    vec3 reflectDir = reflect(-vLightDir, finalNormal);\n"
    "    float specularExp = max(dot(reflectDir, vViewDir), 0.0);\n"
    "    vec3 specular = pow(specularExp, gl_FrontMaterial.shininess) * gl_FrontMaterial.specular.rgb * gl_LightSource[0].specular.rgb;\n"
    "    vec3 ambient = gl_FrontMaterial.ambient.rgb * gl_LightSource[0].ambient.rgb + (diffuseColor.rgb * 0.1);\n"
    "    gl_FragColor = vec4(ambient + diffuse + specular + gl_FrontMaterial.emission.rgb, diffuseColor.a);\n"
    "}";

/**
 * @brief Compila um shader (vertex ou fragment) a partir do código fonte GLSL.
 * @param type Tipo do shader (GL_VERTEX_SHADER ou GL_FRAGMENT_SHADER).
 * @param source Código fonte do shader em string.
 * @return ID do shader compilado ou 0 em caso de erro.
 */
static GLuint compile_shader(GLenum type, const char* source) {
    if (glCreateShader == NULL) {
        fprintf(stderr, "Erro: Funções de Shader não carregadas. Verifique o glewInit.\n");
        return 0;
    }

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Erro ao compilar shader: %s\n", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/**
 * @brief Cria e linka o programa de shader usado para normal mapping nas esferas.
 * @return ID do programa de shader ou 0 em caso de falha.
 */
static GLuint create_sphere_shader_program() {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, sphere_vertex_shader_source);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, sphere_fragment_shader_source);

    if (!vs || !fs) {
        printf("Erro: Shaders não puderam ser criados.\n");
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Erro ao linkar shader program: %s\n", infoLog);
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

/**
 * @brief Obtém uma string de um objeto JSON.
 * @param obj Objeto JSON de origem.
 * @param key Chave a ser buscada.
 * @return String duplicada (malloc) ou NULL se não existir.
 */
char* get_string(cJSON* obj, const char* key) {
    if (!obj) return NULL; // Proteção contra objetos nulos
    cJSON* item = cJSON_GetObjectItem(obj, key);
    return (item && cJSON_IsString(item) && item->valuestring)
       ? strdup(item->valuestring)
       : NULL;
}

/**
 * @brief Obtém um valor float de um objeto JSON.
 * @param obj Objeto JSON de origem.
 * @param key Chave a ser buscada.
 * @return Valor float ou 0.0f se não existir.
 */
float get_float(cJSON* obj, const char* key) {
    if (!obj) return 0.0f; // Proteção
    cJSON* item = cJSON_GetObjectItem(obj, key);
    return item ? (float)item->valuedouble : 0.0f;
}

/**
 * @brief Define valores padrão para um material.
 * @param material Ponteiro para a struct Material a ser inicializada.
 */
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

/**
 * @brief Lê um array RGBA de um JSON e preenche um vetor de floats.
 * @param obj Objeto JSON de origem.
 * @param key Chave do array RGBA.
 * @param out Vetor de saída com 4 componentes (RGBA).
 */
static void read_rgba_array(cJSON* obj, const char* key, GLfloat out[4]) {
    if (!obj) return; // Proteção
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

/**
 * @brief Converte um objeto JSON em uma struct Material.
 * @param material_json Objeto JSON contendo os dados do material.
 * @param type Tipo do corpo (ex: "star").
 * @param name Nome do corpo.
 * @return Estrutura Material preenchida.
 */
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

/**
 * @brief Carrega uma textura de arquivo para o OpenGL.
 * @param filename Caminho do arquivo da textura.
 * @return ID da textura gerada ou 0 em caso de erro.
 */
GLuint loadTexture(const char *filename) {
    int width, height, nrChannels;

    if (!filename || strlen(filename) == 0) {
        printf("Texture inválida\n");
        return 0;
    }

    stbi_set_flip_vertically_on_load(1);

    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        // printf("Erro ao carregar textura: %s\n", filename);
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

/**
 * @brief Carrega todas as texturas dos corpos, luas e anéis.
 * @param bodies Vetor de corpos celestes.
 * @param count Quantidade de corpos no vetor.
 */
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

/**
 * @brief Converte um JSON em uma struct Moon.
 * @param moon_json Objeto JSON da lua.
 * @return Estrutura Moon preenchida.
 */
Moon parse_moon(cJSON* moon_json) {
    Moon moon;
    
    // Inicializar IDs de textura para evitar crashes do OpenGL
    moon.texture_id = 0;
    moon.normal_texture_id = 0;
    moon.secondary_texture_id = 0;

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

/**
 * @brief Converte um JSON em uma struct Rings.
 * @param rings_json Objeto JSON dos anéis.
 * @return Ponteiro para Rings alocado ou NULL.
 */
Rings* parse_rings(cJSON* rings_json) {
    if (!rings_json) return NULL;

    Rings* rings = (Rings *) malloc(sizeof(Rings));
    if (!rings) return NULL; // Prevenção se a memória falhar
    
    rings->texture_id = 0; // Inicializar
    rings->texture_path = get_string(rings_json, "secondary_texture");
    rings->inner_radius = get_float(rings_json, "inner_radius");
    rings->outer_radius = get_float(rings_json, "outer_radius");

    return rings;
}

/**
 * @brief Converte um JSON em uma struct Body (planeta/estrela).
 * @param body_json Objeto JSON do corpo celeste.
 * @return Estrutura Body preenchida.
 */
Body parse_body(cJSON* body_json) {
    Body body;
    
    // Inicializar IDs em zero para que o OpenGL não leia lixo da memória
    body.texture_id = 0;
    body.secondary_texture_id = 0;
    body.normal_texture_id = 0;
    body.specular_texture_id = 0;

    body.name = get_string(body_json, "name");
    body.type = get_string(body_json, "type");

    body.texture_path = get_string(body_json, "texture");
    body.secondary_texture_path = get_string(body_json, "secondary_texture");
    body.normal_texture_path = get_string(body_json, "normal_texture");
    body.specular_texture_path = get_string(body_json, "specular_texture");
    
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
        if (body.moons_count > 0) {
            body.moons = (Moon *) malloc(sizeof(Moon) * body.moons_count);
            for (int i = 0; i < body.moons_count; i++) {
                body.moons[i] = parse_moon(cJSON_GetArrayItem(moons_json, i));
            }
        } else {
            body.moons = NULL;
        }
    } else {
        body.moons = NULL;
        body.moons_count = 0;
    }

    body.rings = parse_rings(cJSON_GetObjectItem(body_json, "rings"));

    return body;
}

/**
 * @brief Busca um corpo pelo nome no vetor de corpos.
 * @param bodies Vetor de corpos.
 * @param count Quantidade de corpos.
 * @param name Nome a ser buscado.
 * @return Ponteiro para o Body encontrado ou NULL.
 */
Body* find_body_by_name(Body* bodies, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (bodies[i].name && name && strcmp(bodies[i].name, name) == 0)
            return &bodies[i];
    }
    return NULL;
}

/**
 * @brief Resolve a hierarquia orbital definindo os pais (orbit_center).
 * @param bodies Vetor de corpos.
 * @param count Quantidade de corpos.
 */
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

/**
 * @brief Carrega configurações globais de escala e iluminação do JSON.
 * @param root Objeto JSON raiz.
 */
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

/**
 * @brief Carrega todos os corpos celestes a partir de um arquivo JSON.
 * @param path Caminho do arquivo JSON.
 * @param out_count Ponteiro para armazenar a quantidade de corpos carregados.
 * @return Vetor alocado de corpos.
 */
Body* load_bodies(const char* path, int* out_count) {
    char* json_data = read_file(path);

    if (!json_data) {
        printf("Erro ao ler arquivo JSON\n");
        exit(1);
    }
    
    cJSON* root = cJSON_Parse(json_data);

    if (!root) {
        printf("Erro: %s\n", cJSON_GetErrorPtr());
        free(json_data); // Prevenir memory leak no caso de erro
        exit(1);
    }
    
    free(json_data); // json_data não é mais necessário após cJSON_Parse

    load_scale(root);

    cJSON* stars_json = cJSON_GetObjectItem(root, "stars");
    if(stars_json)
        stars.texture_path = get_string(stars_json, "texture");

    cJSON* bodies_json = cJSON_GetObjectItem(root, "bodies");

    if (!bodies_json || !cJSON_IsArray(bodies_json)) {
        printf("Erro: 'bodies' inválido\n");
        exit(1);
    }

    int count = cJSON_GetArraySize(bodies_json);
    Body* bodies = (Body *) malloc(sizeof(Body) * count);

    for (int i = 0; i < count; i++) {
        bodies[i] = parse_body(cJSON_GetArrayItem(bodies_json, i));
    }

    resolve_hierarchy(bodies, count);

    // Inicializar o Shader Program para as esferas.
    // Isso é feito uma vez durante o carregamento dos corpos.
    sphere_shader_program = create_sphere_shader_program();

    *out_count = count;
    return bodies;
}

/**
 * @brief Desenha a esfera de fundo com textura de estrelas.
 */
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

/**
 * @brief Desenha a órbita elíptica de um corpo celeste.
 * @param body Ponteiro para o corpo.
 */
void draw_orbit(Body* body) {
    if (!body || body->orbit_radius == 0) return; // Segurança contra body nulo

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

/**
 * @brief Desenha uma esfera com nível de detalhe (LOD) e suporte a normal mapping.
 * @param tex_id ID da textura difusa.
 * @param normal_tex_id ID da textura de normal map.
 * @param radius Raio da esfera.
 * @param x Posição X.
 * @param y Posição Y.
 * @param z Posição Z.
 * @param body_spin Rotação do corpo em torno do próprio eixo.
 */
void draw_sphere_lod(GLuint tex_id, GLuint normal_tex_id, float radius, float x, float y, float z, float body_spin) {
    // 1. Cálculo dinâmico de LOD (Level of Detail)
    float dx = cam.lookFrom.x - x;
    float dy = cam.lookFrom.y - y;
    float dz = cam.lookFrom.z - z;
    float distance = sqrt(dx * dx + dy * dy + dz * dz);
    
    // Proteção para não dividir por zero ou valores muito pequenos
    if (distance < 1.0f) distance = 1.0f;

    // Define a resolução da esfera com base na distância
    int slices = (int)(1000 * radius / distance);
    if (slices < 12) slices = 12;
    if (slices > 100) slices = 100;

    glPushMatrix();
    
    // NOTA: O glTranslatef(x, y, z) deve ser feito ANTES desta função no draw.c
    // para seguir a hierarquia de órbitas corretamente. 
    // Aqui tratamos apenas da rotação local do corpo.
    glRotatef(body_spin, 0.0f, 1.0f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // Alinha o polo da esfera com o eixo Y

    int useShader = 0;
#if BUMP_ACTIVE
    // Só ativa o shader se houver um programa válido e uma textura de normal
    if (sphere_shader_program != 0 && normal_tex_id > 0) {
        useShader = 1;
    }
#endif

    if (useShader) {
        glUseProgram(sphere_shader_program);

        // Unidade de Textura 0: Difusa
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glUniform1i(glGetUniformLocation(sphere_shader_program, "uDiffuseTexture"), 0);

        // Unidade de Textura 1: Normal Map (Bump)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normal_tex_id);
        glUniform1i(glGetUniformLocation(sphere_shader_program, "uNormalMap"), 1);

        glUniform1i(glGetUniformLocation(sphere_shader_program, "uHasNormalMap"), 1);
    } else {
        // Fallback para o Pipeline de Função Fixa (FFP)
        glUseProgram(0);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        
        if (tex_id > 0) {
            glBindTexture(GL_TEXTURE_2D, tex_id);
            // Garante que a cor do material interaja com a textura
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        }
    }

    // Desenha a geometria da esfera
    gluSphere(quad, radius, slices, slices);

    // Limpeza de estado do OpenGL
    if (useShader) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}

/**
 * @brief Desenha os anéis de um planeta com textura.
 * @param rings Ponteiro para a struct Rings.
 * @param planet_radius Raio do planeta base.
 */
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