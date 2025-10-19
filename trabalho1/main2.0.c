/*
  main.c - Versão estendida para o projeto:
  "Compressão e gerenciamento de imagens binárias"
  Implementa:
    - Armazenamento de imagens binarizadas (aplica limiar)
    - Compressão RLE para imagens binárias
    - Arquivo de índices com flags de remoção e limiar utilizado
    - Remoção lógica e compactação do banco (linear)
    - Exportação de imagem (decoding -> PGM)
    - Reconstrução (média de versões binárias) - bônus

  Compilar:
    gcc -std=c11 -Wall -Wextra -O2 main.c -o imgdb

  Uso:
    ./imgdb
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INDEX_FILE "index.bin"
#define DATA_FILE  "imagens.bin"
#define TEMP_INDEX "index.tmp"
#define TEMP_DATA  "imagens.tmp"
#define MAX_NAME   100

typedef struct {
    char nome[MAX_NAME];   // chave/nome da imagem
    long offset;           // offset no arquivo de dados
    int tamanho;           // tamanho em bytes do registro comprimido
    int linhas, colunas;   // dimensão
    int tons;              // número de tons originais (p.ex. 255)
    int limiar;            // limiar usado para gerar a versão binária
    unsigned char removido;// 0 = ativo, 1 = removido
} IndexEntry;

// ---------- Utils ----------
void die_perror(const char *msg) {
    perror(msg);
    exit(1);
}

// Lê PGM P2 (ASCII). Retorna buffer alocado com pixels (0..tons) e preenche linhas/colunas/tons.
// Caller deve free() o buffer.
// Simples: não processa comentários complexos, assume P2 bem formado conforme arquivo de entrada original.
unsigned char *lePgmP2(const char *filename, int *out_linhas, int *out_colunas, int *out_tons) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("Erro ao abrir PGM"); return NULL; }

    char tipo[3];
    if (fscanf(f, "%2s", tipo) != 1) { fclose(f); return NULL; }
    if (strcmp(tipo, "P2") != 0) {
        printf("Formato não suportado. Use P2 (PGM ASCII).\n");
        fclose(f);
        return NULL;
    }

    int colunas = 0, linhas = 0, tons = 0;
    // Pular possíveis comentários
    int c = fgetc(f);
    while (c == '\n' || c == ' ' || c == '\t' || c == '\r') c = fgetc(f);
    if (c == '#') { // comentário, descartar linha
        while (c != '\n' && c != EOF) c = fgetc(f);
    } else {
        ungetc(c, f);
    }

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
    *out_linhas = linhas;
    *out_colunas = colunas;
    *out_tons = tons;
    return pixels;
}

// Escreve PGM P2 a partir de buffer (pixels 0..tons)
int escrevePgmP2(const char *filename, unsigned char *pixels, int linhas, int colunas, int tons) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("Erro ao criar PGM"); return 0; }
    fprintf(f, "P2\n%d %d\n%d\n", colunas, linhas, tons);
    long total = (long)linhas * colunas;
    for (long i = 0; i < total; i++) {
        fprintf(f, "%d ", pixels[i]);
        if ((i + 1) % colunas == 0) fprintf(f, "\n");
    }
    fclose(f);
    return 1;
}

// ---------- Index file helpers ----------
void appendIndexEntry(IndexEntry *entry) {
    FILE *f = fopen(INDEX_FILE, "ab");
    if (!f) die_perror("Erro ao abrir index.bin para append");
    if (fwrite(entry, sizeof(IndexEntry), 1, f) != 1) die_perror("Erro ao escrever index.bin");
    fclose(f);
}

int findIndexByNameAndThreshold(const char *nome, int limiar, IndexEntry *out_entry, long *out_pos) {
    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) return 0;
    IndexEntry e;
    long pos = 0;
    while (fread(&e, sizeof(IndexEntry), 1, f) == 1) {
        if (!e.removido && strcmp(e.nome, nome) == 0 && e.limiar == limiar) {
            if (out_entry) *out_entry = e;
            if (out_pos) *out_pos = pos;
            fclose(f);
            return 1;
        }
        pos += sizeof(IndexEntry);
    }
    fclose(f);
    return 0;
}

void listarImagens() {
    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) { printf("Nenhuma imagem cadastrada.\n"); return; }
    IndexEntry e;
    printf("Imagens cadastradas (não removidas):\n");
    while (fread(&e, sizeof(IndexEntry), 1, f) == 1) {
        if (e.removido) continue; // pular entradas marcadas como removidas
        printf("Nome: %s | limiar: %d | offset: %ld | bytes: %d | %dx%d | tons: %d\n",
               e.nome, e.limiar, e.offset, e.tamanho, e.linhas, e.colunas, e.tons);
    }
    fclose(f);
}

// Marca como removido (remoção lógica). Retorna 1 se encontrou e marcou, 0 caso contrário.
int removerImagem(const char *nome, int limiar) {
    FILE *f = fopen(INDEX_FILE, "r+b");
    if (!f) { printf("Nenhum índice existe ainda.\n"); return 0; }
    IndexEntry e;
    long pos = 0;
    while (fread(&e, sizeof(IndexEntry), 1, f) == 1) {
        if (!e.removido && strcmp(e.nome, nome) == 0 && e.limiar == limiar) {
            e.removido = 1;
            fseek(f, pos, SEEK_SET);
            if (fwrite(&e, sizeof(IndexEntry), 1, f) != 1) die_perror("Erro ao atualizar índice");
            fclose(f);
            return 1;
        }
        pos += sizeof(IndexEntry);
    }
    fclose(f);
    return 0;
}

// ---------- RLE encode/decode for binary images ----------
// Format in data file at given offset:
// 1 byte: starting_tone (0 or 1)
// sequence of uint32_t counts (little-endian as written by fwrite) summing to total_pixels
// No explicit numRuns stored (decoder sums counts until total reached)

void encodeRLE_and_write(unsigned char *binary_pixels, long total_pixels, FILE *f_out, long *out_bytes_written) {
    if (total_pixels <= 0) { *out_bytes_written = 0; return; }
    unsigned char start = binary_pixels[0] ? 1 : 0;
    // write starting tone
    if (fwrite(&start, 1, 1, f_out) != 1) die_perror("Erro ao escrever starting tone");

    uint32_t run = 1;
    long bytes_written = 1;
    for (long i = 1; i < total_pixels; i++) {
        unsigned char bit = binary_pixels[i] ? 1 : 0;
        if (bit == (start ^ (run % 2 ? 0 : 1))) {
            // Not a correct approach: simpler: track current symbol
        }
    }
    // Simpler approach: iterate, count consecutive equal bits
    // Rewind file pointer back to after start and rewrite counts properly.
    // To avoid complexity, we will compute counts in memory then write.

    // Compute counts in memory (vector of uint32_t)
    uint32_t *counts = NULL;
    size_t cap = 0, used = 0;
    long idx = 0;
    while (idx < total_pixels) {
        unsigned char cur = binary_pixels[idx];
        uint32_t cnt = 0;
        while (idx < total_pixels && binary_pixels[idx] == cur) {
            cnt++; idx++;
        }
        if (used == cap) {
            cap = cap ? cap * 2 : 16;
            counts = realloc(counts, cap * sizeof(uint32_t));
            if (!counts) die_perror("realloc");
        }
        counts[used++] = cnt;
    }

    // write counts
    for (size_t i = 0; i < used; i++) {
        uint32_t c = counts[i];
        if (fwrite(&c, sizeof(uint32_t), 1, f_out) != 1) die_perror("Erro ao escrever RLE counts");
        bytes_written += sizeof(uint32_t);
    }
    free(counts);
    *out_bytes_written = bytes_written;
}

// Decoding: reads starting tone then counts until sum == total_pixels
// decoded_pixels must be preallocated with size total_pixels; fills with values 0 or tons
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
        // alternate tone
        cur_tone_bit = !cur_tone_bit;
    }
    return 1;
}

// Helper para ler o registro comprimido de tamanho bytes para um buffer (aloca e retorna)
// usado durante compactação.
unsigned char *readCompressedRecord(FILE *fdata, long offset, int bytes) {
    unsigned char *buf = malloc(bytes);
    if (!buf) die_perror("malloc");
    if (fseek(fdata, offset, SEEK_SET) != 0) die_perror("fseek readCompressedRecord");
    if (fread(buf, 1, bytes, fdata) != (size_t)bytes) die_perror("fread readCompressedRecord");
    return buf;
}

// ---------- Inserção ----------
void inserirImagem(const char *nomeArquivo, const char *nomeChave, int limiar) {
    int linhas, colunas, tons;
    unsigned char *pixels = lePgmP2(nomeArquivo, &linhas, &colunas, &tons);
    if (!pixels) { printf("Falha ao ler PGM de entrada.\n"); return; }

    long total = (long)linhas * colunas;

    // Binarizar com limiar: valor > limiar => 1 (tons), else 0
    unsigned char *binary = malloc(total);
    if (!binary) die_perror("malloc");
    for (long i = 0; i < total; i++) {
        binary[i] = (pixels[i] > limiar) ? 1 : 0; // store as 0/1 for RLE
    }

    // Abrir arquivo de dados e gravar registro comprimido (starting tone + counts)
    FILE *fdata = fopen(DATA_FILE, "ab");
    if (!fdata) die_perror("Erro ao abrir imagens.bin para append");

    if (fseek(fdata, 0, SEEK_END) != 0) die_perror("ftell error");
    long offset = ftell(fdata);

    long bytes_written = 0;
    encodeRLE_and_write(binary, total, fdata, &bytes_written);
    fclose(fdata);

    // Preencher index entry
    IndexEntry e;
    memset(&e, 0, sizeof(IndexEntry));
    strncpy(e.nome, nomeChave, MAX_NAME-1);
    e.offset = offset;
    e.tamanho = (int)bytes_written;
    e.linhas = linhas;
    e.colunas = colunas;
    e.tons = tons;
    e.limiar = limiar;
    e.removido = 0;

    appendIndexEntry(&e);

    free(pixels);
    free(binary);
    printf("Imagem '%s' armazenada (offset=%ld, bytes=%ld, limiar=%d).\n", nomeChave, offset, bytes_written, limiar);
}

// ---------- Exportação (decodifica e escreve PGM) ----------
void exportarImagem(const char *nome, int limiar, int opcao_saida) {
    // opcao_saida: 1 = original (binária armazenada decodificada para tons originais),
    // 2 = negativada (applica tons-valor), 3 = limiarizada (salva imagem binária com tons 0/tons)
    IndexEntry e;
    if (!findIndexByNameAndThreshold(nome, limiar, &e, NULL)) {
        printf("Imagem com nome '%s' e limiar %d não encontrada.\n", nome, limiar);
        return;
    }
    FILE *fdata = fopen(DATA_FILE, "rb");
    if (!fdata) die_perror("Erro ao abrir imagens.bin para leitura");

    long total = (long)e.linhas * e.colunas;
    unsigned char *decoded = malloc(total);
    if (!decoded) die_perror("malloc");
    // decode (fills decoded with 0 or e.tons)
    decodeRLE_from(fdata, e.offset, total, e.tons, decoded);
    fclose(fdata);

    // aplicar opção
    if (opcao_saida == 2) { // negativar
        for (long i = 0; i < total; i++) decoded[i] = (unsigned char)(e.tons - decoded[i]);
    } else if (opcao_saida == 3) {
        // limiarizada: já é binária, mas garantir que valores sejam 0 ou tons
        for (long i = 0; i < total; i++) decoded[i] = (decoded[i] == 0) ? 0 : (unsigned char)e.tons;
    }

    char saida[256];
    if (opcao_saida == 1) sprintf(saida, "%s_export_original.pgm", nome);
    else if (opcao_saida == 2) sprintf(saida, "%s_export_negativada.pgm", nome);
    else sprintf(saida, "%s_export_limiarizada.pgm", nome);

    if (escrevePgmP2(saida, decoded, e.linhas, e.colunas, e.tons))
        printf("Exportado para %s\n", saida);
    else
        printf("Falha ao exportar.\n");

    free(decoded);
}

// ---------- Compactação ----------
void compactarBanco() {
    FILE *fidx = fopen(INDEX_FILE, "rb");
    if (!fidx) { printf("Nenhum índice a compactar.\n"); return; }
    FILE *fdata = fopen(DATA_FILE, "rb");
    if (!fdata) { fclose(fidx); printf("Nenhum arquivo de dados a compactar.\n"); return; }

    FILE *fidx_new = fopen(TEMP_INDEX, "wb");
    if (!fidx_new) die_perror("Erro ao criar índice temporário");
    FILE *fdata_new = fopen(TEMP_DATA, "wb");
    if (!fdata_new) die_perror("Erro ao criar dados temporário");

    IndexEntry e;
    long new_offset = 0;

    while (fread(&e, sizeof(IndexEntry), 1, fidx) == 1) {
        if (e.removido) continue; // pular removidos
        // ler registro comprimido do arquivo antigo
        unsigned char *buf = readCompressedRecord(fdata, e.offset, e.tamanho);
        // gravar no novo arquivo de dados
        if (fseek(fdata_new, 0, SEEK_END) != 0) die_perror("fseek new data");
        long written_offset = ftell(fdata_new);
        if (fwrite(buf, 1, e.tamanho, fdata_new) != (size_t)e.tamanho) die_perror("fwrite new data");
        free(buf);
        // atualizar offset no índice e escrever no índice novo
        e.offset = written_offset;
        if (fwrite(&e, sizeof(IndexEntry), 1, fidx_new) != 1) die_perror("fwrite new index");
    }

    fclose(fidx);
    fclose(fdata);
    fclose(fidx_new);
    fclose(fdata_new);

    // substituir arquivos antigos
    if (remove(INDEX_FILE) != 0) die_perror("remove index old");
    if (remove(DATA_FILE) != 0) die_perror("remove data old");
    if (rename(TEMP_INDEX, INDEX_FILE) != 0) die_perror("rename index");
    if (rename(TEMP_DATA, DATA_FILE) != 0) die_perror("rename data");

    printf("Compactação concluída.\n");
}

// ---------- Reconstrução (média de versões) ----------
void reconstruirMedia(const char *nome) {
    // abrir índice e coletar todas as entradas correspondentes (não removidas) com mesmo nome
    FILE *fidx = fopen(INDEX_FILE, "rb");
    if (!fidx) { printf("Índice não encontrado.\n"); return; }

    IndexEntry e;
    int count = 0;
    // primeiro, contamos e checamos dimensão consistency
    int linhas = -1, colunas = -1, tons = -1;
    // para evitar múltiplas leituras do index, coletamos os matches em um array dinâmico
    IndexEntry *matches = NULL;
    size_t cap = 0, used = 0;
    while (fread(&e, sizeof(IndexEntry), 1, fidx) == 1) {
        if (!e.removido && strcmp(e.nome, nome) == 0) {
            if (used == cap) {
                cap = cap ? cap * 2 : 8;
                matches = realloc(matches, cap * sizeof(IndexEntry));
                if (!matches) die_perror("realloc");
            }
            matches[used++] = e;
            if (linhas == -1) {
                linhas = e.linhas; colunas = e.colunas; tons = e.tons;
            } else {
                if (linhas != e.linhas || colunas != e.colunas) {
                    printf("Encontradas versões com dimensões diferentes. Abortando reconstrução.\n");
                    free(matches); fclose(fidx); return;
                }
            }
        }
    }
    fclose(fidx);
    count = (int)used;
    if (count == 0) {
        printf("Nenhuma versão encontrada para '%s'.\n", nome);
        if (matches) free(matches);
        return;
    }

    long total = (long)linhas * colunas;
    // vetor de soma em ints para evitar overflow
    int *somas = calloc(total, sizeof(int));
    if (!somas) die_perror("calloc");

    FILE *fdata = fopen(DATA_FILE, "rb");
    if (!fdata) die_perror("Erro ao abrir data file para reconstrução");

    unsigned char *decoded = malloc(total);
    if (!decoded) die_perror("malloc");

    for (int i = 0; i < count; i++) {
        IndexEntry me = matches[i];
        // decode this entry
        decodeRLE_from(fdata, me.offset, total, me.tons, decoded);
        for (long p = 0; p < total; p++) {
            somas[p] += (int)decoded[p];
        }
    }
    fclose(fdata);
    free(decoded);
    free(matches);

    // calcular média
    unsigned char *media = malloc(total);
    if (!media) die_perror("malloc");
    for (long p = 0; p < total; p++) {
        // somas[p] está somando valores 0 ou tons. média pode não ser inteiro, arredondamos para inteiro
        double avg = ((double)somas[p]) / (double)count;
        int val = (int)(avg + 0.5); // round
        if (val < 0) val = 0;
        if (val > tons) val = tons;
        media[p] = (unsigned char)val;
    }
    free(somas);

    char saida[256];
    sprintf(saida, "%s_reconstrucao_media.pgm", nome);
    if (escrevePgmP2(saida, media, linhas, colunas, tons))
        printf("Reconstrução média salva em %s (a partir de %d versões).\n", saida, count);
    else
        printf("Falha ao salvar reconstrução.\n");
    free(media);
}

// ---------- Menu ----------
void menu() {
    while (1) {
        printf("\n--- MENU ---\n");
        printf("1 - Inserir imagem (P2) com limiar e compressão RLE\n");
        printf("2 - Listar imagens\n");
        printf("3 - Exportar imagem (decodificar, salvar PGM)\n");
        printf("4 - Exportar imagem negativada\n");
        printf("5 - Exportar imagem limiarizada (binária)\n");
        printf("6 - Remover imagem (marca como removida)\n");
        printf("7 - Compactar banco de imagens (reclaim space)\n");
        printf("8 - Reconstruir média (bônus)\n");
        printf("0 - Sair\n");
        printf("Opcao: ");
        int opc;
        if (scanf("%d", &opc) != 1) { // entrada inválida
            while (getchar()!='\n'); continue;
        }
        if (opc == 0) break;
        if (opc == 1) {
            char arquivo[200], chave[MAX_NAME];
            int limiar;
            printf("Arquivo PGM (P2) de entrada: ");
            scanf("%s", arquivo);
            printf("Nome chave para armazenar: ");
            scanf("%s", chave);
            printf("Valor do limiar (0..tons): ");
            scanf("%d", &limiar);
            inserirImagem(arquivo, chave, limiar);
        } else if (opc == 2) {
            listarImagens();
        } else if (opc == 3 || opc == 4 || opc == 5) {
            char chave[MAX_NAME];
            int limiar;
            printf("Nome da imagem: ");
            scanf("%s", chave);
            printf("Limiar usado na compressão (necessário): ");
            scanf("%d", &limiar);
            if (opc == 3) exportarImagem(chave, limiar, 1);
            else if (opc == 4) exportarImagem(chave, limiar, 2);
            else exportarImagem(chave, limiar, 3);
        } else if (opc == 6) {
            char chave[MAX_NAME];
            int limiar;
            printf("Nome da imagem a remover: ");
            scanf("%s", chave);
            printf("Limiar da versão a remover: ");
            scanf("%d", &limiar);
            if (removerImagem(chave, limiar)) printf("Imagem marcada como removida.\n");
            else printf("Entrada não encontrada.\n");
        } else if (opc == 7) {
            compactarBanco();
        } else if (opc == 8) {
            char chave[MAX_NAME];
            printf("Nome da imagem a reconstruir (usar todas as versões disponíveis): ");
            scanf("%s", chave);
            reconstruirMedia(chave);
        } else {
            printf("Opcao inválida!\n");
        }
    }
}

int main() {
    printf("Banco de imagens binárias com RLE - iniciado.\n");
    menu();
    printf("Encerrando.\n");
    return 0;
}
