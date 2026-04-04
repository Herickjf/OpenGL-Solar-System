#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

    #include "structures.h"

    // Inicializa o sistema de áudio e carrega os arquivos
    void init_audio_controller();

    // Atualiza a música baseada no focused_body ou focused_moon
    void update_audio();

    // Limpa a memória ao fechar o programa
    void close_audio();

#endif