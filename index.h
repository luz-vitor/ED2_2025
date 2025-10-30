/*
Júlia Miranda
Leonardo Serafim
Marcus Vinicius de Oliveira Silva
Patrick Perete Santos 
Vitor Augusto de Campos Luz
*/ 


/**
 * @file index.h
 * @brief Declarações e estruturas para manipulação de índice e ordenação externa.
 *
 * Este cabeçalho define a estrutura IndexEntry e os protótipos das funções
 * para inserir, listar e ordenar imagens armazenadas em arquivos binários.
 *
 * 
 */

#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include <stdbool.h>

/* ---------------------- Constantes ---------------------- */

/** Nome do arquivo binário que armazena o índice */
#define INDEX_FILE "index.bin"

/** Nome do arquivo binário que armazena as imagens */
#define DATA_FILE "imagens.bin"

/** Tamanho máximo do campo 'nome' em cada entrada de índice */
#define MAX_NAME 100

/** Número máximo de entradas carregadas em memória por rodada (ajustável) */
#define MAX_INMEM_ENTRIES 20000


/* ---------------------- Estruturas ---------------------- */

/**
 * @struct IndexEntry
 * @brief Estrutura que representa uma entrada do índice de imagens.
 */
typedef struct {
    char nome[MAX_NAME]; /**< Nome da imagem */
    long offset;          /**< Posição no arquivo de dados */
    int tamanho;          /**< Tamanho em bytes */
    int linhas;           /**< Altura da imagem (pixels) */
    int colunas;          /**< Largura da imagem (pixels) */
    int tons;             /**< Nível máximo de tons de cinza */
} IndexEntry;


/* ---------------------- Protótipos de Funções ---------------------- */

/**
 * @brief Insere uma imagem PGM (P2) no arquivo binário e adiciona ao índice.
 * @param nomeArquivo Caminho do arquivo de imagem PGM.
 * @param nomeChave Nome identificador a ser armazenado no índice.
 */
void inserirImagem(const char *nomeArquivo, const char *nomeChave);

/**
 * @brief Salva uma entrada no arquivo de índice.
 * @param entry Ponteiro para a entrada a ser gravada.
 */
void salvarIndex(IndexEntry *entry);

/**
 * @brief Lista todas as imagens cadastradas no índice.
 */
void listarImagens(void);

/**
 * @brief Gera runs (arquivos temporários) a partir do índice original.
 * @param index_filename Nome do arquivo de índice.
 * @return Número de runs criadas ou -1 em caso de erro.
 */
int generate_runs(const char *index_filename);

/**
 * @brief Executa a intercalação k-way (merge) dos arquivos de runs.
 * @param run_count Número de runs a intercalar.
 * @param out_filename Nome do arquivo de saída ordenado.
 * @return 0 em sucesso, -1 em erro.
 */
int k_way_merge(int run_count, const char *out_filename);

/**
 * @brief Executa o merge sort externo completo sobre o arquivo de índice.
 */
void external_merge_sort_index(void);

#endif /* INDEX_H */
