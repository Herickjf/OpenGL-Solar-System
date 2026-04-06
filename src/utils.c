/*
 * utils.c — Leitura de arquivo binário completo para um buffer terminado em '\0'.
 */
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

/**
 * @brief Lê o conteúdo completo de um arquivo binário e o armazena em um buffer de memória.
 * A função calcula o tamanho do arquivo, aloca a memória necessária e adiciona um caractere 
 * nulo ('\0') ao final para garantir que o conteúdo possa ser tratado como uma string.
 * @param path Caminho do sistema de arquivos para o arquivo a ser lido.
 * @return Um ponteiro para o buffer alocado contendo os dados do arquivo, ou NULL em caso de erro.
 */
char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if(!f){
        printf("Erro ao abrir arquivo: %s\n", path);
        return NULL;
    }

    // encontra final
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
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