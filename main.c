#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char formato[3];
    int largura;
    int altura;
    int maxIntensidade;
    int **pixels;
} PGMImage;

// Função para alocar matriz
int **alocaMatriz(int altura, int largura){
    int **matriz = malloc(altura * sizeof(int *));
    for(int i = 0; i < altura; i++){
        matriz[i] = malloc(largura * sizeof(int)); // ponteiro para o começo da linha i
    }
    return matriz;
}

// Ler imagem PGM
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
            fscanf(fp, "%d", &img->pixels[i][j]);
        }
    }

    fclose(fp);
    return img;
}

// Salvar imagem PGM
void salvarPGM(const char *filename, PGMImage *img){
    FILE *fp = fopen(filename, "w");
    if(!fp){
        perror("Erro ao salvar arquivo PGM");
        exit(1);
    }

    fprintf(fp, "%s \n", img->formato);
    fprintf(fp, "%d %d \n", img->largura, img->altura);
    fprintf(fp, "%d \n", img->maxIntensidade);

    for(int i = 0; i < img->altura; i++){
        for(int j = 0; j < img->largura; j++){
            fprintf(fp, "%d ", img->pixels[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

int main(){
    // Ler imagem original
    PGMImage *img = lerPGM("barbara.pgm");

    // Salvar imagem original
    salvarPGM("barbara_teste.pgm", img);
    return 0;

}