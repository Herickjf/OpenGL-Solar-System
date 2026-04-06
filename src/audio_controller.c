/*
 * audio_controller.c — SDL2_mixer: Gerenciamento de áudio otimizado.
 * Focado em estabilidade e eliminação de stuttering (áudio picotado).
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio_controller.h"
#include "app_state.h"

// ======================
// Parâmetros de Configuração
#define MUSIC_VOLUME 19
#define AUDIO_PATH "audios/"
#define AUDIO_BUFFER 2048 

static Mix_Music* current_music = NULL;
static char last_body_name[32] = "";
static Uint32 last_change_time = 0;

typedef enum { STATE_PLAYING, STATE_STOPPED } AudioState;
static AudioState audio_state = STATE_STOPPED;

int pause_music = 0;

// ======================
// Inicialização
void init_audio_controller() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("[Audio] Erro SDL: %s\n", SDL_GetError());
        return;
    }

    if (Mix_Init(MIX_INIT_MP3) == 0) {
        printf("[Audio] Erro Mix_Init: %s\n", Mix_GetError());
    }

    // Inicializa o dispositivo de áudio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, AUDIO_BUFFER) < 0) {
        printf("[Audio] Erro Mixer: %s\n", Mix_GetError());
        return;
    }

    Mix_VolumeMusic(MUSIC_VOLUME);
    printf("[Audio] Sistema iniciado com Buffer de %d\n", AUDIO_BUFFER);
}

// ======================
// Carregamento e Reprodução (Sem Fades)
static void play_body_music(const char* name) {
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s.mp3", AUDIO_PATH, name);
    
    Mix_Music* next = Mix_LoadMUS(full_path);

    // Fallback para áudio padrão caso o arquivo específico não exista
    if (!next) {
        snprintf(full_path, sizeof(full_path), "%sdefault.mp3", AUDIO_PATH);
        next = Mix_LoadMUS(full_path);
    }

    if (next) {
        // 1. Para o hardware de áudio imediatamente antes de mexer na memória
        Mix_HaltMusic(); 
        
        // 2. Libera a música anterior com segurança
        if (current_music) {
            Mix_FreeMusic(current_music);
            current_music = NULL;
        }

        current_music = next;

        // 3. Reprodução Direta (Loop infinito = -1)
        if (Mix_PlayMusic(current_music, -1) == -1) {
            printf("[Audio] Erro ao tocar %s: %s\n", name, Mix_GetError());
            Mix_FreeMusic(current_music);
            current_music = NULL;
            audio_state = STATE_STOPPED;
        } else {
            audio_state = STATE_PLAYING;
        }
    } else {
        printf("[Audio] Arquivo não encontrado: %s\n", name);
    }
}

// ======================
// Loop de Atualização
void update_audio() {
    // Proteção: verifica se o mixer está ativo
    if (!Mix_QuerySpec(NULL, NULL, NULL)) return;

    const char* current_target = NULL;

    // Prioridade de foco para determinar o áudio
    if (focused_body) {
        current_target = focused_body->name;
    } else if (focused_moon) {
        current_target = focused_moon->name;
    }

    // Gerenciamento de Silêncio/Pausa
    if (pause_music || !current_target) {
        if (Mix_PlayingMusic()) {
            Mix_HaltMusic(); // Parada instantânea evita buffer underruns
            audio_state = STATE_STOPPED;
            last_body_name[0] = '\0'; // Reseta para permitir reativar o mesmo corpo
        }
        return;
    }

    // Troca de música com proteção anti-spam (Debounce)
    Uint32 now = SDL_GetTicks();
    
    // Só troca se o alvo for diferente OU se o áudio estava parado
    // O intervalo de 500ms é vital para não sobrecarregar a thread de áudio
    if ((strcmp(current_target, last_body_name) != 0 || audio_state == STATE_STOPPED) && 
        (now - last_change_time > 500)) {

        last_change_time = now;
        strncpy(last_body_name, current_target, sizeof(last_body_name) - 1);
        last_body_name[sizeof(last_body_name) - 1] = '\0';

        play_body_music(current_target);
    }
}

// ======================
// Encerramento
void close_audio() {
    Mix_HaltMusic();

    if (current_music) {
        Mix_FreeMusic(current_music);
        current_music = NULL;
    }

    Mix_CloseAudio();
    Mix_Quit();
}