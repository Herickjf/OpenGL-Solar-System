/* Teclado, rato e scroll; init_camera_controller alinha câmera livre ao lookAt inicial. */
#ifndef INPUT_H
#define INPUT_H

void init_camera_controller(void);
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);

#endif
