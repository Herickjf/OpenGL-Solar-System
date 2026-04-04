#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio_controller.h"
#include "app_state.h"

// Configurações
#define MUSIC_VOLUME 19         // Volume em ~15% (escala 0-128)
#define AUDIO_PATH "audios/"    // Caminho da pasta de áudios

static Mix_Music* current_music = NULL;
static char last_body_name[32] = "";
int pause_music = 0;

// Inicializa o subsistema de áudio
void init_audio_controller() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("[Audio] Erro SDL: %s\n", SDL_GetError());
        return;
    }

    // 44.1kHz, 16-bit, Stereo, 2048 buffer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("[Audio] Erro Mixer: %s\n", Mix_GetError());
        return;
    }

    Mix_VolumeMusic(MUSIC_VOLUME);
    printf("[Audio] Sistema iniciado. Volume: %d\n", MUSIC_VOLUME);
}

// Tenta carregar e tocar uma música específica
static void play_body_music(const char* name) {
    char full_path[256];
    
    // Tenta carregar o arquivo .mp3 com o nome do corpo
    sprintf(full_path, "%s%s.mp3", AUDIO_PATH, name);
    
    Mix_Music* next = Mix_LoadMUS(full_path);

    // Se falhar, tenta o default.mp3
    if (!next) {
        sprintf(full_path, "%sdefault.mp3", AUDIO_PATH);
        next = Mix_LoadMUS(full_path);
    }

    if (next) {
        // Para a anterior e limpa memória
        if (current_music) {
            Mix_HaltMusic();
            Mix_FreeMusic(current_music);
        }

        current_music = next;
        // O segundo parâmetro -1 indica LOOP INFINITO
        Mix_FadeInMusic(current_music, -1, 2000); 
        printf("[Audio] Tocando agora: %s\n", full_path);
    } else {
        printf("[Audio] Erro: Nenhum arquivo encontrado para %s ou default.\n", name);
    }
}

void update_audio() {

    const char* current_target = NULL;

    if (focused_body) {
        current_target = focused_body->name;
    } else if (focused_moon) {
        current_target = focused_moon->name;
    }

    // Lógica de Pausa (usa a variável pause_music) ou se não há foco
    if (pause_music || !current_target) { 
        if (Mix_PlayingMusic() && !Mix_PausedMusic()) {
            printf("[Audio] Música pausada.\n");
            Mix_FadeOutMusic(1500);
            Mix_PauseMusic();
        }
        return; 
    } else {
        if (Mix_PausedMusic()) {
            printf("[Audio] Música retomada.\n");
            Mix_ResumeMusic();
        }
    }

    // Lógica de Troca de Música (Se há um novo foco)
    if (strcmp(current_target, last_body_name) != 0) {
        strcpy(last_body_name, current_target);
        play_body_music(current_target);
    }
}

void close_audio() {
    if (current_music) {
        Mix_FreeMusic(current_music);
    }
    Mix_CloseAudio();
    SDL_Quit();
}