#include <stdio.h>
#include <stdlib.h>

/**
 * Estrutura que representa uma imagem PGM.
 * formato: "P2" para texto ou "P5" para binário.
 * largura: número de colunas (pixels por linha).
 * altura: número de linhas (pixels por coluna).
 * maxIntensidade: valor máximo de intensidade do pixel.
 * pixels: matriz dinâmica de pixels (unsigned char).
 */
typedef struct {
    char formato[3];
    int largura;
    int altura;
    int maxIntensidade;
    unsigned char **pixels;
} PGMImage;

/**
 * Aloca uma matriz dinâmica de pixels.
 * @param altura Número de linhas.
 * @param largura Número de colunas.
 * @return Ponteiro para matriz alocada.
 */
unsigned char **alocaMatriz(int altura, int largura){
    unsigned char **matriz = malloc(altura * sizeof(unsigned char *));
    for(int i = 0; i < altura; i++){
        matriz[i] = malloc(largura * sizeof(unsigned char));
    }
    return matriz;
}

/**
 * Lê uma imagem PGM no formato texto (P2).
 * @param filename Caminho do arquivo PGM.
 * @return Ponteiro para estrutura PGMImage preenchida.
 */
PGMImage *lerPGM(const char *filename){
    FILE *fp = fopen(filename, "r");
    if (!fp){
        printf("Erro ao abrir o arquivo PGM");
        exit(1);
    }

    PGMImage *img = malloc(sizeof(PGMImage));
    fscanf(fp, "%s", img->formato);
    fscanf(fp, "%d %d", &img->largura, &img->altura);
    fscanf(fp, "%d", &img->maxIntensidade);

    img->pixels = alocaMatriz(img->altura, img->largura);

    for(int i = 0; i < img->altura; i++){
        for(int j = 0; j < img->largura; j++){
            fscanf(fp, "%hhu", &img->pixels[i][j]);
        }
    }

    fclose(fp);
    return img;
}

/**
 * Salva uma imagem PGM no formato texto (P2).
 * @param filename Caminho do arquivo de saída.
 * @param img Ponteiro para estrutura PGMImage.
 */
void salvarPGM(const char *filename, PGMImage *img){
    FILE *fp = fopen(filename, "w");
    if(!fp){
        perror("Erro ao salvar arquivo PGM");
        exit(1);
    }

    fprintf(fp, "%s\n", img->formato);
    fprintf(fp, "%d %d\n", img->largura, img->altura);
    fprintf(fp, "%d\n", img->maxIntensidade);

    for(int i = 0; i < img->altura; i++){
        for(int j = 0; j < img->largura; j++){
            fprintf(fp, "%hhu ", img->pixels[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

/**
 * Salva uma imagem PGM no formato binário (P5).
 * @param filename Caminho do arquivo de saída.
 * @param img Ponteiro para estrutura PGMImage.
 */
void salvarPGMBinario(const char *filename, PGMImage *img) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Erro ao salvar PGM binário");
        exit(1);
    }

    // Cabeçalho P5
    fprintf(fp, "P5\n%d %d\n%d\n", img->largura, img->altura, img->maxIntensidade);

    // Escreve os pixels linha a linha
    for (int i = 0; i < img->altura; i++) {
        fwrite(img->pixels[i], sizeof(unsigned char), img->largura, fp);
    }

    fclose(fp);
}

/**
 * Lê uma imagem PGM no formato binário (P5).
 * @param filename Caminho do arquivo PGM binário.
 * @return Ponteiro para estrutura PGMImage preenchida.
 */
PGMImage *lerPGMBinario(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Erro ao abrir PGM binário");
        exit(1);
    }

    PGMImage *img = malloc(sizeof(PGMImage));
    fscanf(fp, "%s", img->formato);
    fscanf(fp, "%d %d", &img->largura, &img->altura);
    fscanf(fp, "%d", &img->maxIntensidade);
    fgetc(fp); // consome o '\n' após maxIntensidade

    img->pixels = alocaMatriz(img->altura, img->largura);

    // Lê os pixels binários linha a linha
    for (int i = 0; i < img->altura; i++) {
        fread(img->pixels[i], sizeof(unsigned char), img->largura, fp);
    }

    fclose(fp);
    return img;
}

/**
 * Aplica limiarização à imagem.
 * Pixels abaixo do limiar L recebem valor 0, acima ou igual recebem maxIntensidade.
 * @param img Ponteiro para estrutura PGMImage.
 * @param L Valor do limiar.
 */
void limiarizacao(PGMImage *img, int L) {
    for (int i = 0; i < img->altura; i++) {
        for (int j = 0; j < img->largura; j++) {
            img->pixels[i][j] = (img->pixels[i][j] < L) ? 0 : img->maxIntensidade;
        }
    }
}

int main() {
    // const char *inputTxt = "imagens/barbara.pgm";
    // const char *outputBin = "imagens/barbara_binaria.pgm";
    // const char *outputLimiarizado = "imagens/barbara_limiarizada.pgm";
    // int limiar = 128;

    const char *inputTxt = "imagens/exemplo.pgm";
    const char *outputBin = "imagens/exemplo_binaria.pgm";
    const char *outputLimiarizado = "imagens/exemplo_limiarizada.pgm";
    int limiar = 128;
    
    // Passo 1: Ler imagem PGM em texto (P2)
    PGMImage *imgTxt = lerPGM(inputTxt);

    // Passo 2: Salvar em binário (P5)
    salvarPGMBinario(outputBin, imgTxt);

    // Passo 3: Ler de volta o arquivo binário
    PGMImage *imgBin = lerPGMBinario(outputBin);

    // Passo 4: Aplicar limiarização
    limiarizacao(imgBin, limiar);

    // Passo 5: Salvar resultado em binário
    salvarPGMBinario(outputLimiarizado, imgBin);

    return 0;
}