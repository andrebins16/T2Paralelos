#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <time.h>

#define LARGURA_BASE 4000
#define ALTURA_BASE 4000
#define MAX_ITERACOES 1000
#define EPSILON 1e-6

#define X_MIN -0.05
#define X_MAX  0.05
#define Y_MIN -0.05
#define Y_MAX  0.05


int calcula_convergencia(complex double z) {
    for (int i = 0; i < MAX_ITERACOES; i++) {
        complex double f = cpow(z, 3) - 1; //calcula a função
        complex double f_linha = 3 * cpow(z, 2); //calcula derivada
        if (cabs(f) < EPSILON) { //convergiu (encontrou uma raíz), então retorna o número de iterações
            return i;
        }
        z = z - f / f_linha; // próximo passo segundo o método de  Newton-Raphson --> x_(n+1) = x_n - f(x_n) / f'(x_n) 
    }
    return MAX_ITERACOES; // não convergiu
}

void salvar_matriz_em_arquivo(
    int **matriz, int largura, int altura, double tempo_execucao,
    double x_min, double x_max, double y_min, double y_max,
    const char *arquivo_saida
) {
    FILE *fp = fopen(arquivo_saida, "w");
    if (!fp) {
        perror("Erro ao abrir arquivo de saída");
        exit(1);
    }

    // Cabeçalho
    fprintf(fp, "%d %d %.4f %.17f %.17f %.17f %.17f\n",
            largura, altura, tempo_execucao, x_min, x_max, y_min, y_max);

    // Matriz
    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            fprintf(fp, "%d", matriz[y][x]);
            if (x < largura - 1) fprintf(fp, " ");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void gerar_fractal_newton(int largura, int altura, const char *arquivo_saida) {
    int **matriz = malloc(altura * sizeof(int *));
    if (!matriz) {
        perror("Erro ao alocar linhas da matriz");
        exit(1);
    }

    for (int i = 0; i < altura; i++) {
        matriz[i] = malloc(largura * sizeof(int));
        if (!matriz[i]) {
            perror("Erro ao alocar colunas da matriz");
            for (int j = 0; j < i; j++) free(matriz[j]);
            free(matriz);
            exit(1);
        }
    }

    clock_t inicio = clock();


    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) { //para cada ponto 
            double real = X_MIN + (X_MAX - X_MIN) * x / (largura - 1);
            double imaginario = Y_MIN + (Y_MAX - Y_MIN) * y / (altura - 1);
            complex double z = real + imaginario * I; //transforma o ponto em um numero complexo dentro da área determinada 
            matriz[y][x] = calcula_convergencia(z); //calcula a convergencia para o ponto 
        }
    }

    clock_t fim = clock();
    double tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;

    salvar_matriz_em_arquivo(
        matriz, largura, altura, tempo,
        X_MIN, X_MAX, Y_MIN, Y_MAX,
        arquivo_saida
    );

    for (int i = 0; i < altura; i++) free(matriz[i]);
    free(matriz);

    printf("Tempo de execução: %.4f segundos\n", tempo);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <quantidade_cores_simuladas>\n", argv[0]);
        return 1;
    }

    int nucleos = atoi(argv[1]);
    if (nucleos <= 0) {
        fprintf(stderr, "Valor inválido para quantidade de núcleos.\n");
        return 1;
    }

    int largura = LARGURA_BASE * nucleos;

    char nome_arquivo[100];
    snprintf(nome_arquivo, sizeof(nome_arquivo), "newton_seq_%d_output.dat", nucleos);

    printf("Execução sequencial com multiplicador de cores = %d\n", nucleos);
    gerar_fractal_newton(largura, ALTURA_BASE, nome_arquivo);

    return 0;
}