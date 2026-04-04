#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

    #include <GL/glut.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    // Rings (Saturn, etc.)
    typedef struct {
        char* texture_path;
        GLuint texture_id;

        float inner_radius;
        float outer_radius;
    } Rings;

    typedef struct {
        char* name;

        char* texture_path;
        char* normal_texture_path;
        char* secondary_texture_path;

        GLuint texture_id;
        GLuint normal_texture_id;
        GLuint secondary_texture_id;

        float radius;
        float orbit_radius;
        float eccentricity;
        float orbit_inclination;
        float axial_tilt;
        float orbital_period;
        float rotation_period;
    } Moon;

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

    typedef struct {
        float x, y, z;
    } Position;

    typedef struct {
        Position lookFrom;
        Position lookAt;
        Position vUp;
    } Camera;

    typedef enum {
        CAMERA_FREE,
        CAMERA_FOLLOW,
        CAMERA_ORBIT
    } CameraMode;


#endif