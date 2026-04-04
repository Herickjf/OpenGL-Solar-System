#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio_controller.h"
#include "app_state.h"

// ======================
// Configurações
#define MUSIC_VOLUME 19
#define AUDIO_PATH "audios/"
#define AUDIO_BUFFER 4096   // equilíbrio: menos crepitação sem pesar

static Mix_Music* current_music = NULL;
static char last_body_name[32] = "";
static Uint32 last_change_time = 0;

int pause_music = 0;

// ======================
// Inicialização
void init_audio_controller() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("[Audio] Erro SDL: %s\n", SDL_GetError());
        return;
    }

    // Inicializa suporte a MP3
    if (Mix_Init(MIX_INIT_MP3) == 0) {
        printf("[Audio] Erro Mix_Init: %s\n", Mix_GetError());
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, AUDIO_BUFFER) < 0) {
        printf("[Audio] Erro Mixer: %s\n", Mix_GetError());
        return;
    }

    Mix_VolumeMusic(MUSIC_VOLUME);

    printf("[Audio] Sistema iniciado. Buffer: %d | Volume: %d\n",
           AUDIO_BUFFER, MUSIC_VOLUME);
}

// ======================
// Tocar música
static void play_body_music(const char* name) {
    char full_path[256];

    // tenta arquivo do corpo
    snprintf(full_path, sizeof(full_path), "%s%s.mp3", AUDIO_PATH, name);
    Mix_Music* next = Mix_LoadMUS(full_path);

    // fallback
    if (!next) {
        snprintf(full_path, sizeof(full_path), "%sdefault.mp3", AUDIO_PATH);
        next = Mix_LoadMUS(full_path);
    }

    if (next) {
        if (current_music) {
            Mix_HaltMusic();
            Mix_FreeMusic(current_music);
        }

        current_music = next;

        // fade suave de entrada
        Mix_FadeInMusic(current_music, -1, 1000);

        printf("[Audio] Tocando: %s\n", full_path);
    } else {
        printf("[Audio] Erro: não encontrou áudio para %s\n", name);
    }
}

// ======================
// Update principal
void update_audio() {
    const char* current_target = NULL;

    if (focused_body) {
        current_target = focused_body->name;
    } else if (focused_moon) {
        current_target = focused_moon->name;
    }

    // ======================
    // PAUSA
    if (pause_music || !current_target) {
        if (Mix_PlayingMusic() && !Mix_PausedMusic()) {
            printf("[Audio] Pausando...\n");

            // apenas fadeout (sem pause junto pra evitar glitch)
            Mix_FadeOutMusic(1000);
        }
        return;
    }

    // retoma se pausado
    if (Mix_PausedMusic()) {
        printf("[Audio] Retomando...\n");
        Mix_ResumeMusic();
    }

    // ======================
    // TROCA DE MÚSICA (com proteção anti-spam)
    Uint32 now = SDL_GetTicks();

    if (strcmp(current_target, last_body_name) != 0 &&
        now - last_change_time > 500) {

        last_change_time = now;
        strncpy(last_body_name, current_target, sizeof(last_body_name) - 1);
        last_body_name[sizeof(last_body_name) - 1] = '\0';

        play_body_music(current_target);
    }
}

// ======================
// Cleanup
void close_audio() {
    if (current_music) {
        Mix_FreeMusic(current_music);
    }

    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
}