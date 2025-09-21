/*
Júlia Miranda
Leonardo Serafim
Marcus Vinicius de Oliveira Silva
Patrick Perete Santos 
Vitor Augusto de Campos Luz
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_FILE "index.bin"
#define DATA_FILE "imagens.bin"
#define MAX_NAME 100

typedef struct {
    char nome[MAX_NAME];
    long offset;
    int tamanho;
    int linhas, colunas, tons;
} IndexEntry;

// ---------- Funções utilitárias ----------
void salvarIndex(IndexEntry *entry) {
    FILE *f = fopen(INDEX_FILE, "ab");
    if (!f) { perror("Erro ao abrir index.dat"); exit(1); }
    fwrite(entry, sizeof(IndexEntry), 1, f);
    fclose(f);
}

void listarImagens() {
    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) { printf("Nenhuma imagem cadastrada.\n"); return; }
    IndexEntry e;
    printf("Imagens cadastradas:\n");
    while (fread(&e, sizeof(IndexEntry), 1, f)) {
        printf("Nome: %s | Offset: %ld | Tamanho: %d bytes\n", e.nome, e.offset, e.tamanho);
    }
    fclose(f);
}

int carregarIndexPorNome(const char *nome, IndexEntry *entry) {
    FILE *f = fopen(INDEX_FILE, "rb");
    if (!f) return 0;
    while (fread(entry, sizeof(IndexEntry), 1, f)) {
        if (strcmp(entry->nome, nome) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// ---------- Operações de processamento ----------
void negativar(unsigned char *dados, int total, int tons) {
    for (int i = 0; i < total; i++)
        dados[i] = tons - dados[i];
}

void limiarizar(unsigned char *dados, int total, int tons) {
    int limiar = tons / 2;
    for (int i = 0; i < total; i++)
        dados[i] = (dados[i] > limiar) ? tons : 0;
}

// ---------- Inserção ----------
void inserirImagem(const char *nomeArquivo, const char *nomeChave) {
    FILE *fin = fopen(nomeArquivo, "rb");
    if (!fin) { perror("Erro ao abrir imagem"); return; }

    char tipo[3];
    int linhas, colunas, tons;
    fscanf(fin, "%2s", tipo);
    if (strcmp(tipo, "P2") != 0) {
        printf("Formato não suportado! Use P2 (PGM ASCII).\n");
        fclose(fin);
        return;
    }
    fscanf(fin, "%d %d %d", &colunas, &linhas, &tons);

    unsigned char *pixels = malloc(linhas * colunas);
    for (int i = 0; i < linhas * colunas; i++) {
        int val;
        fscanf(fin, "%d", &val);
        pixels[i] = (unsigned char) val;
    }
    fclose(fin);

    FILE *fdata = fopen(DATA_FILE, "ab");
    if (!fdata) { perror("Erro ao abrir imagens.dat"); exit(1); }

    fseek(fdata, 0, SEEK_END);
    long offset = ftell(fdata);
    fwrite(pixels, 1, linhas * colunas, fdata);
    fclose(fdata);

    IndexEntry e;
    strcpy(e.nome, nomeChave);
    e.offset = offset;
    e.tamanho = linhas * colunas;
    e.linhas = linhas;
    e.colunas = colunas;
    e.tons = tons;
    salvarIndex(&e);

    free(pixels);
    printf("Imagem %s armazenada com sucesso!\n", nomeChave);
}

// ---------- Recuperação ----------
void exportarImagem(const char *nomeChave, int opcao) {
    IndexEntry e;
    if (!carregarIndexPorNome(nomeChave, &e)) {
        printf("Imagem não encontrada!\n");
        return;
    }

    FILE *fdata = fopen(DATA_FILE, "rb");
    if (!fdata) { perror("Erro ao abrir imagens.dat"); return; }

    fseek(fdata, e.offset, SEEK_SET);
    unsigned char *pixels = malloc(e.tamanho);
    fread(pixels, 1, e.tamanho, fdata);
    fclose(fdata);

    if (opcao == 2) negativar(pixels, e.tamanho, e.tons);
    else if (opcao == 3) limiarizar(pixels, e.tamanho, e.tons);

    char saida[120];
    if(opcao == 2){
        sprintf(saida, "%s_negativada.pgm", nomeChave);
    }else if (opcao == 3){
        sprintf(saida, "%s_limiarizada.pgm", nomeChave);
    }
    
    FILE *fout = fopen(saida, "w");
    fprintf(fout, "P2\n%d %d\n%d\n", e.colunas, e.linhas, e.tons);
    for (int i = 0; i < e.tamanho; i++) {
        fprintf(fout, "%d ", pixels[i]);
        if ((i + 1) % e.colunas == 0) fprintf(fout, "\n");
    }
    fclose(fout);

    free(pixels);
    printf("Imagem exportada como %s\n", saida);
}

// ---------- Menu principal ----------
int main() {
    int opcao;
    char arquivo[MAX_NAME], chave[MAX_NAME];

    while (1) {
        printf("\n--- MENU ---\n");
        printf("1 - Inserir imagem\n");
        printf("2 - Listar imagens\n");
        printf("3 - Exportar imagem (original)\n");
        printf("4 - Exportar imagem (negativada)\n");
        printf("5 - Exportar imagem (limiarizada)\n");
        printf("0 - Sair\n");
        printf("Opcao: ");
        scanf("%d", &opcao);

        if (opcao == 0) break;
        switch (opcao) {
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
                printf("Nome da imagem: ");
                scanf("%s", chave);
                exportarImagem(chave, 1);
                break;
            case 4:
                printf("Nome da imagem: ");
                scanf("%s", chave);
                exportarImagem(chave, 2);
                break;
            case 5:
                printf("Nome da imagem: ");
                scanf("%s", chave);
                exportarImagem(chave, 3);
                break;
            default:
                printf("Opcao invalida!\n");
        }
    }
    return 0;
}
