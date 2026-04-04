#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "app_state.h"

// Estrutura para controle de zoom e suavização
extern float camera_zoom;

// Função principal de atualização (chamada no update do sistema)
void update_camera(float delta_time);

// Funções utilitárias de interpolação
float lerp(float a, float b, float t);
Position lerp_pos(Position a, Position b, float t);

#endif