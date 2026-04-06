/* Estruturas da simulação: anéis, materiais, luas, corpos (Body), fundo de estrelas, câmera e CameraMode. */
#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

    #include <GL/glut.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    /**
     * @struct Rings
     * @brief Define as propriedades dos anéis planetários (ex: Saturno).
     * Contém os caminhos de textura e as dimensões físicas dos anéis.
     */
    typedef struct {
        char* texture_path;
        GLuint texture_id;

        float inner_radius;
        float outer_radius;
    } Rings;

    /**
     * @struct Material
     * @brief Define as propriedades do material de um corpo celeste.
     * Contém as componentes difusa, especular, emissiva e a brilhantidade.
     */
    typedef struct {
        GLfloat diffuse[4];
        GLfloat specular[4];
        GLfloat emission[4];
        GLfloat shininess;
    } Material;

    /**
     * @struct Moon
     * @brief Define as propriedades de uma lua (ex: Lua de Júpiter).
     * Contém informações sobre texturas, dimensões e órbita.
     */
    typedef struct {
        char* name;

        char* texture_path;
        char* normal_texture_path;
        char* secondary_texture_path;

        GLuint texture_id;
        GLuint normal_texture_id;
        GLuint secondary_texture_id;

        Material material;

        float radius;
        float orbit_radius;
        float eccentricity;
        float orbit_inclination;
        float axial_tilt;
        float orbital_period;
        float rotation_period;
    } Moon;

    /**
     * @struct Body
     * @brief Define as propriedades de um corpo celeste (ex: planeta, estrela).
     * Contém informações sobre texturas, dimensões, órbita e propriedades físicas.
     */
    typedef struct Body {
        char* name;
        char* type;

        char* texture_path;
        char* secondary_texture_path;
        char* normal_texture_path;
        char* specular_texture_path;

        GLuint texture_id;
        GLuint secondary_texture_id;
        GLuint normal_texture_id;
        GLuint specular_texture_id;

        Material material;

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

    /**
     * @struct Stars
     * @brief Define as propriedades do plano de fundo estrelado.
     * Contém o caminho e o ID da textura usada para renderizar as estrelas.
     */
    typedef struct {
        char* texture_path;
        GLuint texture_id;
    } Stars;

    /**
     * @struct Position
     * @brief Define as propriedades de uma posição 3D.
     */
    typedef struct {
        float x, y, z;
    } Position;

    /**
     * @struct Camera
     * @brief Define as propriedades da câmera.
     */
    typedef struct {
        Position lookFrom;
        Position lookAt;
        Position vUp;
    } Camera;

    /**
     * @enum CameraMode
     * @brief Define os modos de controle da câmera.
     * CAMERA_FREE: controle total pelo usuário.
     * CAMERA_FOLLOW: segue um corpo específico.
     * CAMERA_ORBIT: orbita em torno de um corpo específico.
     */
    typedef enum {
        CAMERA_FREE,
        CAMERA_FOLLOW,
        CAMERA_ORBIT
    } CameraMode;


#endif