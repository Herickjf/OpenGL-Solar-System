#ifndef APP_STATE_H
#define APP_STATE_H

typedef struct {
    float x, y, z;
} Position;

typedef struct {
    Position lookFrom;
    Position lookAt;
    Position vUp;
} Camera;

extern Camera cam;
extern float time_scale;

#endif
