#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if(!f){
        printf("Erro ao abrir arquivo: %s\n", path);
        return NULL;
    }

    // encontra final
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    // aloca espaço pro buffer (+1 para o \0)
    char* buffer = (char*) malloc(sz+1);
    if(!buffer){
        printf("Erro de alocacao de memoria\n");
        fclose(f);
        return(NULL);
    }

    size_t read_size = fread(buffer, 1, sz, f);
    if(read_size != sz){
        printf("Erro ao ler arquivo\n");
        free(buffer);
        fclose(f);
        return NULL;
    }

    buffer[sz] = '\0';

    fclose(f);
    return buffer;
}