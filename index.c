/*
Júlia Miranda
Leonardo Serafim
Marcus Vinicius de Oliveira Silva
Patrick Perete Santos 
Vitor Augusto de Campos Luz
*/ 

/**
 * @file index.c
 * @brief Implementação das funções de manipulação de índice e ordenação externa.
 *
 * Este módulo contém as funções que permitem inserir imagens,
 * gerar o arquivo de índice e ordená-lo por meio de um Merge Sort Externo.
 *
 * 
 */

#include "index.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ---------------------- Funções Utilitárias ---------------------- */

void salvarIndex(IndexEntry *entry) {
    FILE *f = fopen(INDEX_FILE, "ab");
    if (!f) {
        perror("salvarIndex: fopen");
        return;
    }
    if (fwrite(entry, sizeof(IndexEntry), 1, f) != 1)
        perror("salvarIndex: fwrite");
    fclose(f);
}

void listarImagens(void) {
    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) {
        printf("Nenhum indice encontrado (%s)\n", INDEX_FILE);
        return;
    }

    IndexEntry e;
    printf("Imagens cadastradas:\n");
    while (fread(&e, sizeof(IndexEntry), 1, f) == 1) {
        printf("Nome: %s | Offset: %ld | Tamanho: %d bytes | %dx%d (tons=%d)\n",
               e.nome, e.offset, e.tamanho, e.linhas, e.colunas, e.tons);
    }
    fclose(f);
}

void inserirImagem(const char *nomeArquivo, const char *nomeChave) {
    FILE *fin = fopen(nomeArquivo, "rb");
    if (!fin) { perror("Erro ao abrir imagem (entrada)"); return; }

    char tipo[3];
    int linhas, colunas, tons;
    if (fscanf(fin, "%2s", tipo) != 1 || strcmp(tipo, "P2") != 0) {
        printf("Formato não suportado! Use P2 (PGM ASCII).\n");
        fclose(fin);
        return;
    }
    if (fscanf(fin, "%d %d %d", &colunas, &linhas, &tons) != 3) {
        printf("Cabeçalho inválido\n");
        fclose(fin);
        return;
    }

    long npixels = (long)linhas * colunas;
    unsigned char *pixels = malloc(npixels);
    if (!pixels) { perror("malloc pixels"); fclose(fin); return; }

    for (long i = 0; i < npixels; i++) {
        int val;
        if (fscanf(fin, "%d", &val) != 1) val = 0;
        pixels[i] = (unsigned char)val;
    }
    fclose(fin);

    FILE *fdata = fopen(DATA_FILE, "ab");
    if (!fdata) { perror("Erro ao abrir imagens.bin"); free(pixels); return; }

    fseek(fdata, 0, SEEK_END);
    long offset = ftell(fdata);
    fwrite(pixels, 1, npixels, fdata);
    fclose(fdata);

    IndexEntry e;
    strncpy(e.nome, nomeChave, MAX_NAME - 1);
    e.nome[MAX_NAME - 1] = '\0';
    e.offset = offset;
    e.tamanho = (int)npixels;
    e.linhas = linhas;
    e.colunas = colunas;
    e.tons = tons;

    salvarIndex(&e);
    free(pixels);

    printf("Imagem %s armazenada com sucesso!\n", nomeChave);
}

/* ---------------------- Merge Sort Externo ---------------------- */

static int cmp_indexentry(const void *a, const void *b) {
    return strcmp(((IndexEntry*)a)->nome, ((IndexEntry*)b)->nome);
}

int generate_runs(const char *index_filename) {
    FILE *fin = fopen(index_filename, "rb");
    if (!fin) {
        fprintf(stderr, "generate_runs: erro ao abrir '%s': %s\n", index_filename, strerror(errno));
        return -1;
    }

    IndexEntry *buffer = malloc(sizeof(IndexEntry) * MAX_INMEM_ENTRIES);
    if (!buffer) {
        fprintf(stderr, "generate_runs: falha ao alocar %zu bytes para buffer: %s\n",
                sizeof(IndexEntry) * (size_t)MAX_INMEM_ENTRIES, strerror(errno));
        fclose(fin);
        return -1;
    }

    int run_count = 0;
    while (1) {
        size_t read = fread(buffer, sizeof(IndexEntry), MAX_INMEM_ENTRIES, fin);
        if (read == 0) {
            if (ferror(fin)) {
                fprintf(stderr, "generate_runs: erro ao ler '%s': %s\n", index_filename, strerror(errno));
                free(buffer);
                fclose(fin);
                return -1;
            }
            break; /* EOF */
        }

        qsort(buffer, read, sizeof(IndexEntry), cmp_indexentry);

        char runname[128];
        snprintf(runname, sizeof(runname), "run_%03d.bin", run_count);
        FILE *frun = fopen(runname, "wb");
        if (!frun) {
            fprintf(stderr, "generate_runs: erro ao criar run '%s': %s\n", runname, strerror(errno));
            free(buffer);
            fclose(fin);
            return -1;
        }
        size_t written = fwrite(buffer, sizeof(IndexEntry), read, frun);
        if (written != read) {
            fprintf(stderr, "generate_runs: falha ao escrever run '%s' (escreveu %zu de %zu): %s\n",
                    runname, written, read, strerror(errno));
            fclose(frun);
            free(buffer);
            fclose(fin);
            return -1;
        }
        fclose(frun);

        run_count++;
        if (read < MAX_INMEM_ENTRIES) break;
    }

    free(buffer);
    fclose(fin);
    return run_count;
}

/* --- Funções auxiliares do heap (internas) --- */

static void heapify_down(int *heap, int heap_size, int i, IndexEntry *current);
static void heap_push(int *heap, int *heap_size, int run_idx, IndexEntry *current);
static int heap_pop(int *heap, int *heap_size, IndexEntry *current);

static void heap_push(int *heap, int *heap_size, int run_idx, IndexEntry *current) {
    int i = (*heap_size)++;
    heap[i] = run_idx;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (strcmp(current[heap[i]].nome, current[heap[p]].nome) < 0) {
            int tmp = heap[i]; heap[i] = heap[p]; heap[p] = tmp;
            i = p;
        } else break;
    }
}

static void heapify_down(int *heap, int heap_size, int i, IndexEntry *current) {
    while (1) {
        int l = 2*i + 1, r = 2*i + 2, smallest = i;
        if (l < heap_size && strcmp(current[heap[l]].nome, current[heap[smallest]].nome) < 0) smallest = l;
        if (r < heap_size && strcmp(current[heap[r]].nome, current[heap[smallest]].nome) < 0) smallest = r;
        if (smallest != i) {
            int tmp = heap[i]; heap[i] = heap[smallest]; heap[smallest] = tmp;
            i = smallest;
        } else break;
    }
}

static int heap_pop(int *heap, int *heap_size, IndexEntry *current) {
    if (*heap_size == 0) return -1;
    int root = heap[0];
    (*heap_size)--;
    if (*heap_size > 0) {
        heap[0] = heap[*heap_size];
        heapify_down(heap, *heap_size, 0, current);
    }
    return root;
}

int k_way_merge(int run_count, const char *out_filename) {
    if (run_count <= 0) return -1;

    FILE **runs = NULL;
    IndexEntry *current = NULL;
    bool *has_current = NULL;
    int *heap = NULL;
    FILE *fout = NULL;

    runs = malloc(sizeof(FILE*) * run_count);
    current = malloc(sizeof(IndexEntry) * run_count);
    has_current = malloc(sizeof(bool) * run_count);
    heap = malloc(sizeof(int) * run_count);
    if (!runs || !current || !has_current || !heap) {
        fprintf(stderr, "k_way_merge: malloc falhou (%s)\n", strerror(errno));
        goto cleanup;
    }

    for (int i = 0; i < run_count; i++) {
        char runname[128];
        snprintf(runname, sizeof(runname), "run_%03d.bin", i);
        runs[i] = fopen(runname, "rb");
        if (!runs[i]) {
            fprintf(stderr, "k_way_merge: nao foi possivel abrir run '%s' (idx=%d): %s\n",
                    runname, i, strerror(errno));
            goto cleanup;
        }

        if (fread(&current[i], sizeof(IndexEntry), 1, runs[i]) == 1) {
            has_current[i] = true;
        } else {
            if (ferror(runs[i])) {
                fprintf(stderr, "k_way_merge: erro ao ler run '%s' (idx=%d): %s\n",
                        runname, i, strerror(errno));
                goto cleanup;
            }
            
            has_current[i] = false;
            rewind(runs[i]);
        }
    }

    int heap_size = 0;
    for (int i = 0; i < run_count; i++) if (has_current[i]) heap_push(heap, &heap_size, i, current);

    fout = fopen(out_filename, "wb");
    if (!fout) {
        fprintf(stderr, "k_way_merge: nao foi possivel criar saida '%s': %s\n", out_filename, strerror(errno));
        goto cleanup;
    }

    while (heap_size > 0) {
        int idx = heap_pop(heap, &heap_size, current);
        if (idx < 0) break;

        if (fwrite(&current[idx], sizeof(IndexEntry), 1, fout) != 1) {
            fprintf(stderr, "k_way_merge: falha ao escrever em '%s': %s\n", out_filename, strerror(errno));
            goto cleanup;
        }

        if (fread(&current[idx], sizeof(IndexEntry), 1, runs[idx]) == 1) {
            heap_push(heap, &heap_size, idx, current);
        } else {
            if (ferror(runs[idx])) {
                char runname[128];
                snprintf(runname, sizeof(runname), "run_%03d.bin", idx);
                fprintf(stderr, "k_way_merge: erro ao ler run '%s' (idx=%d): %s\n",
                        runname, idx, strerror(errno));
                goto cleanup;
            }
            
            fclose(runs[idx]);
            runs[idx] = NULL;
        }
    }
    
    for (int i = 0; i < run_count; i++) if (runs[i]) fclose(runs[i]);
    fclose(fout);
    free(runs); free(current); free(has_current); free(heap);
    return 0;

cleanup:
   
    if (fout) { fclose(fout); fout = NULL; remove(out_filename); }
    if (runs) {
        for (int i = 0; i < run_count; i++) if (runs[i]) fclose(runs[i]);
    }
    free(runs); free(current); free(has_current); free(heap);
    return -1;
}

void external_merge_sort_index(void) {
    printf("Gerando corridas (runs)...\n");
    int runs = generate_runs(INDEX_FILE);
    if (runs <= 0) { printf("Nada a ordenar.\n"); return; }

    printf("Intercalando %d runs...\n", runs);
    const char *tmp = "index_sorted.bin";
    if (k_way_merge(runs, tmp) != 0) { printf("Erro na intercalação.\n"); return; }

    remove(INDEX_FILE);
    rename(tmp, INDEX_FILE);

    for (int i = 0; i < runs; i++) {
        char runname[128];
        snprintf(runname, sizeof(runname), "run_%03d.bin", i);
        remove(runname);
    }
    printf("Ordenação concluída!\n");
}
