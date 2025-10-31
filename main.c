/*
Júlia Miranda
Leonardo Serafim
Marcus Vinicius de Oliveira Silva
Patrick Perete Santos 
Vitor Augusto de Campos Luz
*/ 

/**
 * @file main.c
 * @brief Programa principal
 *
 * Permite inserir imagens PGM, listar o índice e ordenar
 * o arquivo de índice usando Merge Sort Externo.
 * As funções de negativar, limiarizar e exportarImagem, foram retiradas do contexto dessa atividade.
 */

#include "index.h"
#include <stdio.h>

int main(void) {
    int opcao;
    char arquivo[MAX_NAME], chave[MAX_NAME];

    while (1) {
        printf("\n--- MENU ---\n");
        printf("1 - Inserir imagem\n");
        printf("2 - Listar imagens\n");
        printf("3 - Ordenar índice\n");
        printf("0 - Sair\n");
        printf("Opção: ");

        if (scanf("%d", &opcao) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (opcao) {
            case 0:
                return 0;
            case 1:
                printf("Arquivo PGM (P2) de entrada: ");
                scanf("%s", arquivo);
                printf("Nome chave para armazenar: ");
                scanf("%s", chave);
                inserirImagem(arquivo, chave);
                break;
            case 2:
                listarImagens();
                break;
            case 3:
                external_merge_sort_index();
                break;
            default:
                printf("Opção inválida!\n");
        }
    }
}
