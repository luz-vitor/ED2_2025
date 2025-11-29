/*
Trabalho 2 - Estruturas de Dados II
Autor: Vitor Luz
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define DATA_FILE  "imagens.bin"
#define INDEX_FILE "btree_index.bin"
#define MAX_NAME   100
#define ORDEM      3 // ordem da árvore-B

// Estrutura para cada imagem binarizada (chave da árvore-B)
typedef struct {
    char nome[MAX_NAME];
    int limiar;
    long offset;
    int tamanho;
    int linhas, colunas, tons;
} Chave;

// Estrutura de página da árvore-B
typedef struct {
    int n; // número de chaves
    Chave chaves[ORDEM-1];
    long filhos[ORDEM]; // offsets para filhos (0 se folha)
    int folha;
    long self_offset; // offset da página no arquivo
} PaginaB;

// ---------- Utils ----------
/*
 * Imprime mensagem de erro e encerra o programa.
 */
void die_perror(const char *msg) {
    perror(msg);
    exit(1);
}

// ---------- Persistência do offset da raiz ----------
/*
 * Salva o offset da raiz no início do arquivo índice.
 */
void salvaRootOffset(FILE *fidx, long root_offset) {
    fseek(fidx, 0, SEEK_SET);
    fwrite(&root_offset, sizeof(long), 1, fidx);
    fflush(fidx);
}

/*
 * Lê o offset da raiz do início do arquivo índice.
 */
long carregaRootOffset(FILE *fidx) {
    long root_offset = 0;
    fseek(fidx, 0, SEEK_SET);
    fread(&root_offset, sizeof(long), 1, fidx);
    return root_offset;
}

// ---------- Árvore-B helpers ----------
/*
 * Compara duas chaves (nome + limiar).
 */
int comparaChave(const Chave *a, const Chave *b) {
    int cmp = strcmp(a->nome, b->nome);
    if (cmp != 0) return cmp;
    return a->limiar - b->limiar;
}

/*
 * Cria uma nova página (folha ou interna) no arquivo.
 */
long criaPagina(FILE *f, int folha) {
    PaginaB pag;
    memset(&pag, 0, sizeof(PaginaB));
    pag.folha = folha;
    fseek(f, 0, SEEK_END);
    pag.self_offset = ftell(f);
    fwrite(&pag, sizeof(PaginaB), 1, f);
    fflush(f);
    return pag.self_offset;
}

/*
 * Cria página e salva root_offset no início do arquivo índice.
 */
long criaPaginaComRoot(FILE *fidx, int folha) {
    fseek(fidx, sizeof(long), SEEK_SET); // pula root_offset
    PaginaB pag;
    memset(&pag, 0, sizeof(PaginaB));
    pag.folha = folha;
    pag.self_offset = ftell(fidx);
    fwrite(&pag, sizeof(PaginaB), 1, fidx);
    fflush(fidx);
    salvaRootOffset(fidx, pag.self_offset);
    return pag.self_offset;
}

/*
 * Lê uma página da árvore-B do arquivo.
 */
void lerPagina(FILE *f, long offset, PaginaB *pag) {
    fseek(f, offset, SEEK_SET);
    fread(pag, sizeof(PaginaB), 1, f);
}

/*
 * Escreve uma página da árvore-B no arquivo.
 */
void escrevePagina(FILE *f, PaginaB *pag) {
    fseek(f, pag->self_offset, SEEK_SET);
    fwrite(pag, sizeof(PaginaB), 1, f);
    fflush(f);
}

/*
 * Imprime uma página da árvore-B de forma amigável.
 */
void imprimePagina(PaginaB *pag, int num) {
    printf("\n--- Página %d (%s) ---\n", num, pag->folha ? "folha" : "interna");
    printf("%-20s %-8s %-8s %-8s\n", "Nome", "Limiar", "Dimensões", "Offset");
    for (int i = 0; i < pag->n; i++)
        printf("%-20s %-8d %4dx%-4d %-8ld\n",
            pag->chaves[i].nome,
            pag->chaves[i].limiar,
            pag->chaves[i].linhas,
            pag->chaves[i].colunas,
            pag->chaves[i].offset);
}

/*
 * Percorre toda a árvore-B e imprime as páginas ordenadamente.
 */
void percursoImpressao(FILE *f, long offset, int *num_pag) {
    if (offset == 0) return;
    PaginaB pag;
    lerPagina(f, offset, &pag);
    imprimePagina(&pag, (*num_pag)++);
    for (int i = 0; i <= pag.n; i++) {
        if (pag.filhos[i])
            percursoImpressao(f, pag.filhos[i], num_pag);
    }
}

/*
 * Busca uma chave (nome+limiar) na árvore-B.
 */
int buscaB(FILE *f, long offset, const char *nome, int limiar, Chave *out) {
    if (offset == 0) return 0;
    PaginaB pag;
    lerPagina(f, offset, &pag);
    int i = 0;
    while (i < pag.n && (strcmp(nome, pag.chaves[i].nome) > 0 ||
                         (strcmp(nome, pag.chaves[i].nome) == 0 && limiar > pag.chaves[i].limiar)))
        i++;
    if (i < pag.n && strcmp(nome, pag.chaves[i].nome) == 0 && limiar == pag.chaves[i].limiar) {
        if (out) *out = pag.chaves[i];
        return 1;
    }
    if (pag.folha) return 0;
    return buscaB(f, pag.filhos[i], nome, limiar, out);
}

/*
 * Realiza o split de uma página cheia (ordem 3).
 */
long splitPagina(FILE *f, PaginaB *pag, Chave *chave_promovida, long *filho_direito) {
    Chave temp_chaves[ORDEM];
    long temp_filhos[ORDEM+1];
    for (int i = 0; i < ORDEM-1; i++) temp_chaves[i] = pag->chaves[i];
    for (int i = 0; i < ORDEM; i++) temp_filhos[i] = pag->filhos[i];

    PaginaB nova;
    memset(&nova, 0, sizeof(PaginaB));
    nova.folha = pag->folha;
    nova.self_offset = criaPagina(f, pag->folha);

    nova.chaves[0] = temp_chaves[2];
    nova.n = 1;
    if (!pag->folha) {
        nova.filhos[0] = temp_filhos[2];
        nova.filhos[1] = temp_filhos[3];
    }

    *chave_promovida = temp_chaves[1];
    *filho_direito = nova.self_offset;

    pag->chaves[0] = temp_chaves[0];
    pag->n = 1;
    if (!pag->folha) {
        pag->filhos[0] = temp_filhos[0];
        pag->filhos[1] = temp_filhos[1];
    }
    escrevePagina(f, pag);
    escrevePagina(f, &nova);

    return nova.self_offset;
}

/*
 * Insere uma chave na árvore-B (recursivo, com split).
 */
int insereRec(FILE *f, long offset, Chave nova, Chave *chave_promovida, long *filho_direito) {
    PaginaB pag;
    lerPagina(f, offset, &pag);

    Chave temp_chaves[ORDEM];
    long temp_filhos[ORDEM+1];
    int temp_n = pag.n;

    for (int i = 0; i < pag.n; i++) temp_chaves[i] = pag.chaves[i];
    for (int i = 0; i <= pag.n; i++) temp_filhos[i] = pag.filhos[i];

    int i = temp_n - 1;
    while (i >= 0 && comparaChave(&nova, &temp_chaves[i]) < 0)
        i--;

    if (pag.folha) {
        if (temp_n < ORDEM-1) {
            for (int j = temp_n; j > i+1; j--)
                temp_chaves[j] = temp_chaves[j-1];
            temp_chaves[i+1] = nova;
            temp_n++;
            for (int j = 0; j < temp_n; j++) pag.chaves[j] = temp_chaves[j];
            pag.n = temp_n;
            escrevePagina(f, &pag);
            return 0;
        } else {
            for (int j = temp_n; j > i+1; j--)
                temp_chaves[j] = temp_chaves[j-1];
            temp_chaves[i+1] = nova;
            temp_n++;
            pag.chaves[0] = temp_chaves[0];
            pag.n = 1;
            if (!pag.folha) {
                pag.filhos[0] = temp_filhos[0];
                pag.filhos[1] = temp_filhos[1];
            }
            escrevePagina(f, &pag);

            PaginaB nova;
            memset(&nova, 0, sizeof(PaginaB));
            nova.folha = pag.folha;
            nova.self_offset = criaPagina(f, pag.folha);
            nova.chaves[0] = temp_chaves[2];
            nova.n = 1;
            if (!pag.folha) {
                nova.filhos[0] = temp_filhos[2];
                nova.filhos[1] = temp_filhos[3];
            }
            escrevePagina(f, &nova);

            *chave_promovida = temp_chaves[1];
            *filho_direito = nova.self_offset;
            return 1;
        }
    } else {
        int filho_idx = i+1;
        Chave promovida;
        long filho_dir;
        int split = insereRec(f, temp_filhos[filho_idx], nova, &promovida, &filho_dir);
        if (!split) {
            for (int j = 0; j <= pag.n; j++) pag.filhos[j] = temp_filhos[j];
            escrevePagina(f, &pag);
            return 0;
        }
        if (temp_n < ORDEM-1) {
            for (int j = temp_n; j > filho_idx; j--) {
                temp_chaves[j] = temp_chaves[j-1];
                temp_filhos[j+1] = temp_filhos[j];
            }
            temp_chaves[filho_idx] = promovida;
            temp_filhos[filho_idx+1] = filho_dir;
            temp_n++;
            for (int j = 0; j < temp_n; j++) pag.chaves[j] = temp_chaves[j];
            for (int j = 0; j <= temp_n; j++) pag.filhos[j] = temp_filhos[j];
            pag.n = temp_n;
            escrevePagina(f, &pag);
            return 0;
        } else {
            for (int j = temp_n; j > filho_idx; j--) {
                temp_chaves[j] = temp_chaves[j-1];
                temp_filhos[j+1] = temp_filhos[j];
            }
            temp_chaves[filho_idx] = promovida;
            temp_filhos[filho_idx+1] = filho_dir;
            temp_n++;
            pag.chaves[0] = temp_chaves[0];
            pag.n = 1;
            pag.filhos[0] = temp_filhos[0];
            pag.filhos[1] = temp_filhos[1];
            escrevePagina(f, &pag);

            PaginaB nova;
            memset(&nova, 0, sizeof(PaginaB));
            nova.folha = pag.folha;
            nova.self_offset = criaPagina(f, pag.folha);
            nova.chaves[0] = temp_chaves[2];
            nova.n = 1;
            nova.filhos[0] = temp_filhos[2];
            nova.filhos[1] = temp_filhos[3];
            escrevePagina(f, &nova);

            *chave_promovida = temp_chaves[1];
            *filho_direito = nova.self_offset;
            return 1;
        }
    }
}

/*
 * Insere uma chave na árvore-B, criando nova raiz se necessário.
 */
long insereB(FILE *f, long root_offset, Chave nova) {
    Chave promovida;
    long filho_dir;
    int split = insereRec(f, root_offset, nova, &promovida, &filho_dir);
    if (!split) return root_offset;
    PaginaB nova_raiz;
    memset(&nova_raiz, 0, sizeof(PaginaB));
    nova_raiz.folha = 0;
    nova_raiz.n = 1;
    nova_raiz.chaves[0] = promovida;
    nova_raiz.filhos[0] = root_offset;
    nova_raiz.filhos[1] = filho_dir;
    fseek(f, 0, SEEK_END);
    nova_raiz.self_offset = ftell(f);
    fwrite(&nova_raiz, sizeof(PaginaB), 1, f);
    fflush(f);
    salvaRootOffset(f, nova_raiz.self_offset); // salva novo root
    return nova_raiz.self_offset;
}

/*
 * Remove uma chave da árvore-B (remoção simples, sem merge).
 */
int removeRec(FILE *f, long offset, const char *nome, int limiar) {
    PaginaB pag;
    lerPagina(f, offset, &pag);
    int i = 0;
    while (i < pag.n && (strcmp(nome, pag.chaves[i].nome) > 0 ||
                         (strcmp(nome, pag.chaves[i].nome) == 0 && limiar > pag.chaves[i].limiar)))
        i++;
    if (i < pag.n && strcmp(nome, pag.chaves[i].nome) == 0 && limiar == pag.chaves[i].limiar) {
        for (int j = i; j < pag.n-1; j++)
            pag.chaves[j] = pag.chaves[j+1];
        pag.n--;
        escrevePagina(f, &pag);
        return 1;
    }
    if (pag.folha) return 0;
    return removeRec(f, pag.filhos[i], nome, limiar);
}

// ---------- Imagem helpers ----------
/*
 * Lê uma imagem PGM (P2) e retorna buffer alocado com pixels.
 */
unsigned char *lePgm(const char *filename, int *out_linhas, int *out_colunas, int *out_tons) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("Erro ao abrir PGM"); return NULL; }
    char tipo[3];
    if (fscanf(f, "%2s", tipo) != 1) { fclose(f); return NULL; }
    if (strcmp(tipo, "P2") != 0) { fclose(f); return NULL; }
    int colunas = 0, linhas = 0, tons = 0;
    int c = fgetc(f);
    while (c == '\n' || c == ' ' || c == '\t' || c == '\r') c = fgetc(f);
    if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); }
    else ungetc(c, f);
    if (fscanf(f, "%d %d", &colunas, &linhas) != 2) { fclose(f); return NULL; }
    if (fscanf(f, "%d", &tons) != 1) { fclose(f); return NULL; }
    long total = (long)linhas * colunas;
    unsigned char *pixels = malloc(total);
    if (!pixels) die_perror("malloc");
    for (long i = 0; i < total; i++) {
        int v;
        if (fscanf(f, "%d", &v) != 1) { free(pixels); fclose(f); return NULL; }
        pixels[i] = (unsigned char) v;
    }
    fclose(f);
    *out_linhas = linhas; *out_colunas = colunas; *out_tons = tons;
    return pixels;
}

// ---------- RLE encode/decode ----------
/*
 * Codifica pixels binários em RLE e salva no arquivo.
 */
void encodeRLE_and_write(unsigned char *binary_pixels, long total_pixels, FILE *f_out, long *out_bytes_written) {
    if (total_pixels <= 0) { *out_bytes_written = 0; return; }
    unsigned char start = binary_pixels[0] ? 1 : 0;
    fwrite(&start, 1, 1, f_out);
    long bytes_written = 1;
    long idx = 0;
    while (idx < total_pixels) {
        unsigned char cur = binary_pixels[idx];
        uint32_t cnt = 0;
        while (idx < total_pixels && binary_pixels[idx] == cur) { cnt++; idx++; }
        fwrite(&cnt, sizeof(uint32_t), 1, f_out);
        bytes_written += sizeof(uint32_t);
    }
    *out_bytes_written = bytes_written;
}

/*
 * Decodifica RLE de uma imagem binária do arquivo.
 */
int decodeRLE_from(FILE *f_in, long offset, int total_pixels, int tons, unsigned char *decoded_pixels) {
    if (total_pixels <= 0) return 0;
    if (fseek(f_in, offset, SEEK_SET) != 0) die_perror("fseek decode");
    unsigned char start;
    if (fread(&start, 1, 1, f_in) != 1) die_perror("Erro ao ler starting tone");
    int cur_tone_bit = start ? 1 : 0;
    long filled = 0;
    while (filled < total_pixels) {
        uint32_t cnt;
        if (fread(&cnt, sizeof(uint32_t), 1, f_in) != 1) die_perror("Erro ao ler count RLE");
        for (uint32_t k = 0; k < cnt; k++) {
            decoded_pixels[filled++] = cur_tone_bit ? (unsigned char)tons : 0;
        }
        cur_tone_bit = !cur_tone_bit;
    }
    return 1;
}

// ---------- Inserção com múltiplos limiares ----------
/*
 * Insere uma imagem PGM com múltiplos limiares no banco.
 */
void inserirImagemMultLimiares(const char *nomeArquivo, const char *nomeChave, int *limiares, int n_limiares, FILE *fdata, FILE *fidx, long *root_offset) {
    int linhas, colunas, tons;
    unsigned char *pixels = lePgm(nomeArquivo, &linhas, &colunas, &tons);
    if (!pixels) { printf("Falha ao ler PGM de entrada.\n"); return; }
    long total = (long)linhas * colunas;
    for (int l = 0; l < n_limiares; l++) {
        int limiar = limiares[l];
        unsigned char *binary = malloc(total);
        if (!binary) die_perror("malloc");
        for (long i = 0; i < total; i++) binary[i] = (pixels[i] > limiar) ? 1 : 0;
        fseek(fdata, 0, SEEK_END);
        long offset = ftell(fdata);
        long bytes_written = 0;
        encodeRLE_and_write(binary, total, fdata, &bytes_written);
        Chave nova;
        memset(&nova, 0, sizeof(Chave));
        strncpy(nova.nome, nomeChave, MAX_NAME-1);
        nova.nome[MAX_NAME-1] = '\0';
        nova.limiar = limiar;
        nova.offset = offset;
        nova.tamanho = (int)bytes_written;
        nova.linhas = linhas;
        nova.colunas = colunas;
        nova.tons = tons;
        *root_offset = insereB(fidx, *root_offset, nova);
        salvaRootOffset(fidx, *root_offset); // salva root após cada inserção
        free(binary);
        printf("Imagem '%s' inserida com limiar %d.\n", nomeChave, limiar);
    }
    free(pixels);
}

// ---------- Compactação recursiva ----------
/*
 * Percorre toda a árvore-B e copia registros ativos para novo arquivo de dados.
 */
void compactaRec(FILE *fidx, FILE *fdata, FILE *fdata_new, long offset) {
    if (offset == 0) return;
    PaginaB pag;
    lerPagina(fidx, offset, &pag);
    for (int i = 0; i < pag.n; i++) {
        Chave *c = &pag.chaves[i];
        unsigned char *buf = malloc(c->tamanho);
        fseek(fdata, c->offset, SEEK_SET);
        fread(buf, 1, c->tamanho, fdata);
        fseek(fdata_new, 0, SEEK_END);
        long new_offset = ftell(fdata_new);
        fwrite(buf, 1, c->tamanho, fdata_new);
        free(buf);
        c->offset = new_offset; // atualiza offset na árvore
    }
    for (int i = 0; i <= pag.n; i++) {
        if (pag.filhos[i])
            compactaRec(fidx, fdata, fdata_new, pag.filhos[i]);
    }
}

/*
 * Compacta o banco de imagens, removendo registros marcados como removidos.
 */
void compactarBanco(FILE *fidx, long root_offset) {
    printf("\nCompactando banco de imagens...\n");
    FILE *fdata = fopen(DATA_FILE, "rb");
    if (!fdata) { printf("Nenhum arquivo de dados a compactar.\n"); return; }
    FILE *fdata_new = fopen("imagens.tmp", "wb");
    if (!fdata_new) die_perror("Erro ao criar dados temporário");

    compactaRec(fidx, fdata, fdata_new, root_offset);

    fclose(fdata);
    fclose(fdata_new);
    remove(DATA_FILE);
    rename("imagens.tmp", DATA_FILE);
    printf("Compactação concluída!\n");
}

// ---------- Reconstrução média ----------
/*
 * Coleta todas as versões de uma imagem (mesmo nome) na árvore-B.
 */
void coletaChavesRec(FILE *fidx, long offset, const char *nome, Chave **out, int *count, int *cap, int *linhas, int *colunas, int *tons) {
    if (offset == 0) return;
    PaginaB pag;
    lerPagina(fidx, offset, &pag);
    for (int i = 0; i < pag.n; i++) {
        Chave *c = &pag.chaves[i];
        if (strcmp(c->nome, nome) == 0) {
            if (*count == *cap) {
                *cap = *cap ? (*cap)*2 : 8;
                *out = realloc(*out, (*cap)*sizeof(Chave));
                if (!*out) die_perror("realloc");
            }
            (*out)[(*count)++] = *c;
            if (*linhas == -1) {
                *linhas = c->linhas; *colunas = c->colunas; *tons = c->tons;
            } else {
                if (*linhas != c->linhas || *colunas != c->colunas) {
                    printf("Versões com dimensões diferentes. Abortando reconstrução.\n");
                    return;
                }
            }
        }
    }
    for (int i = 0; i <= pag.n; i++) {
        if (pag.filhos[i])
            coletaChavesRec(fidx, pag.filhos[i], nome, out, count, cap, linhas, colunas, tons);
    }
}

/*
 * Reconstrói a imagem pela média das versões binarizadas.
 */
void reconstruirMedia(FILE *fidx, long root_offset, const char *nome) {
    printf("\nReconstruindo média da imagem '%s'...\n", nome);
    int linhas = -1, colunas = -1, tons = -1;
    Chave *matches = NULL;
    int cap = 0, count = 0;
    coletaChavesRec(fidx, root_offset, nome, &matches, &count, &cap, &linhas, &colunas, &tons);
    if (count == 0) {
        printf("Nenhuma versão encontrada para '%s'.\n", nome);
        if (matches) free(matches);
        return;
    }
    long total = (long)linhas * colunas;
    int *somas = calloc(total, sizeof(int));
    if (!somas) die_perror("calloc");
    FILE *fdata = fopen(DATA_FILE, "rb");
    if (!fdata) die_perror("Erro ao abrir data file para reconstrução");
    unsigned char *decoded = malloc(total);
    if (!decoded) die_perror("malloc");
    for (int i = 0; i < count; i++) {
        Chave me = matches[i];
        decodeRLE_from(fdata, me.offset, total, me.tons, decoded);
        for (long p = 0; p < total; p++) {
            somas[p] += (int)decoded[p];
        }
    }
    fclose(fdata);
    free(decoded);
    free(matches);
    unsigned char *media = malloc(total);
    if (!media) die_perror("malloc");
    for (long p = 0; p < total; p++) {
        double avg = ((double)somas[p]) / (double)count;
        int val = (int)(avg + 0.5);
        if (val < 0) val = 0;
        if (val > tons) val = tons;
        media[p] = (unsigned char)val;
    }
    free(somas);
    char saida[256];
    sprintf(saida, "%s_reconstrucao_media.pgm", nome);
    FILE *fout = fopen(saida, "w");
    if (!fout) { perror("Erro ao criar PGM"); free(media); return; }
    fprintf(fout, "P2\n%d %d\n%d\n", colunas, linhas, tons);
    for (long i = 0; i < total; i++) {
        fprintf(fout, "%d ", media[i]);
        if ((i + 1) % colunas == 0) fprintf(fout, "\n");
    }
    fclose(fout);
    printf("Reconstrução média salva em '%s' (a partir de %d versões).\n", saida, count);
    free(media);
}

// ---------- Reorganização da árvore ----------
/*
 * Percorre toda a árvore-B e coleta todas as chaves ativas em um vetor.
 */
void coletaChavesParaSort(FILE *fidx, long offset, Chave **out, int *count, int *cap) {
    if (offset == 0) return;
    PaginaB pag;
    lerPagina(fidx, offset, &pag);
    for (int i = 0; i < pag.n; i++) {
        if (*count == *cap) {
            *cap = *cap ? (*cap)*2 : 32;
            *out = realloc(*out, (*cap)*sizeof(Chave));
            if (!*out) die_perror("realloc");
        }
        (*out)[(*count)++] = pag.chaves[i];
    }
    for (int i = 0; i <= pag.n; i++) {
        if (pag.filhos[i])
            coletaChavesParaSort(fidx, pag.filhos[i], out, count, cap);
    }
}

/*
 * Função de comparação para qsort.
 */
int comparaChaveSort(const void *a, const void *b) {
    const Chave *ca = (const Chave *)a;
    const Chave *cb = (const Chave *)b;
    int cmp = strcmp(ca->nome, cb->nome);
    if (cmp != 0) return cmp;
    return ca->limiar - cb->limiar;
}

/*
 * Remove todas as páginas da árvore-B do arquivo índice, exceto o header.
 */
void limpaPaginas(FILE *fidx) {
    fseek(fidx, sizeof(long), SEEK_SET);
    // Trunca o arquivo para só conter o header
    fflush(fidx);
    int fd = fileno(fidx);
    ftruncate(fd, sizeof(long));
    fflush(fidx);
}

/*
 * Reorganiza a árvore-B: coleta todas as chaves, ordena e insere em nova árvore.
 */
void reorganizaArvore(FILE *fidx, long *root_offset) {
    printf("\nReorganizando árvore...\n");
    Chave *vetor = NULL;
    int cap = 0, count = 0;
    coletaChavesParaSort(fidx, *root_offset, &vetor, &count, &cap);
    if (count == 0) {
        printf("Nenhuma chave ativa para reorganizar.\n");
        free(vetor);
        return;
    }
    qsort(vetor, count, sizeof(Chave), comparaChaveSort);

    limpaPaginas(fidx);
    *root_offset = criaPaginaComRoot(fidx, 1);

    for (int i = 0; i < count; i++) {
        *root_offset = insereB(fidx, *root_offset, vetor[i]);
        salvaRootOffset(fidx, *root_offset);
    }
    free(vetor);
    printf("Árvore reorganizada e ordenada!\n");
}

// ---------- Menu principal ----------

void menu(long *root_offset) {
    FILE *fidx = fopen(INDEX_FILE, "r+b");
    if (!fidx) {
        fidx = fopen(INDEX_FILE, "w+b");
        *root_offset = criaPaginaComRoot(fidx, 1);
    } else {
        *root_offset = carregaRootOffset(fidx);
    }
    FILE *fdata = fopen(DATA_FILE, "ab+");
    if (!fdata) die_perror("Erro ao abrir imagens.bin");
    while (1) {
        printf("\n====================\n");
        printf("BANCO DE IMAGENS BINÁRIAS\n");
        printf("====================\n");
        printf("1 - Inserir imagem (PGM) com múltiplos limiares\n");
        printf("2 - Imprimir páginas da Árvore-B\n");
        printf("3 - Remover imagem\n");
        printf("4 - Compactar banco de imagens\n");
        printf("5 - Reconstruir média\n");
        printf("6 - Reorganizar árvore (sort)\n");
        printf("0 - Sair\n");
        printf("--------------------\n");
        printf("Escolha uma opção: ");
        int opc;
        if (scanf("%d", &opc) != 1) { while (getchar()!='\n'); continue; }
        if (opc == 0) break;
        if (opc == 1) {
            char arquivo[200], chave[MAX_NAME];
            int limiares[10], n_lims = 0;
            printf("\nArquivo PGM (P2) de entrada: ");
            scanf("%s", arquivo);
            printf("Nome chave para armazenar: ");
            scanf("%s", chave);
            printf("Quantos limiares? ");
            scanf("%d", &n_lims);
            for (int i = 0; i < n_lims; i++) {
                printf("Limiar %d: ", i+1);
                scanf("%d", &limiares[i]);
            }
            inserirImagemMultLimiares(arquivo, chave, limiares, n_lims, fdata, fidx, root_offset);
        } else if (opc == 2) {
            printf("\nIMPRESSÃO DAS PÁGINAS DA ÁRVORE-B\n");
            int num_pag = 1;
            percursoImpressao(fidx, *root_offset, &num_pag);
        } else if (opc == 3) {
            char chave[MAX_NAME];
            int limiar;
            printf("\nNome chave para remover: ");
            scanf("%s", chave);
            printf("Limiar da imagem: ");
            scanf("%d", &limiar);
            int ok = removeRec(fidx, *root_offset, chave, limiar);
            if (ok) printf("Remoção realizada com sucesso!\n");
            else printf("Erro: imagem não encontrada.\n");
        } else if (opc == 4) {
            compactarBanco(fidx, *root_offset);
        } else if (opc == 5) {
            char chave[MAX_NAME];
            printf("\nNome da imagem a reconstruir (usar todas as versões disponíveis): ");
            scanf("%s", chave);
            reconstruirMedia(fidx, *root_offset, chave);
        } else if (opc == 6) {
            reorganizaArvore(fidx, root_offset);
        }
        else {
            printf("Opção inválida!\n");
        }
    }
    fclose(fidx);
    fclose(fdata);
}

/*
 * Função principal do programa.
 */
int main() {
    printf("\n====================\n");
    printf("BANCO DE IMAGENS BINÁRIAS COM ÁRVORE-B\n");
    printf("====================\n");
    long root_offset = 0;
    menu(&root_offset);
    printf("\nEncerrando programa.\n");
    return 0;
}