#ifndef BODIES_H
#define BODIES_H

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../libs/cJSON.h"
#include "structures.h"

// externos
extern GLUquadric *quad;

extern float distance_scale;
extern float radius_scale;
extern float time_scale;

extern Stars stars;

// helpers
char* get_string(cJSON* obj, const char* key);
float get_float(cJSON* obj, const char* key);

// textures / load
GLuint loadTexture(const char *filename);
void load_all_textures(Body* bodies, int count);

// parsing
Moon parse_moon(cJSON* moon_json);
Rings* parse_rings(cJSON* rings_json);
Body parse_body(cJSON* body_json);

// utils
Body* find_body_by_name(Body* bodies, int count, const char* name);
void resolve_hierarchy(Body* bodies, int count);
void load_scale(cJSON* root);
Body* load_bodies(const char* path, int* out_count);

// draw
void draw_stars_background();
void draw_orbit(Body* body);
void draw_sphere_lod(float radius, float x, float y, float z, float body_spin);
void draw_rings(Rings* rings, float planet_radius);

#endif