#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <time.h>


#define WEAK_CORES 32
#define WIDTH  1920
#define HEIGHT 1080
#define MAX_ITER 1000 * WEAK_CORES


#define X_MIN -1.5
#define X_MAX -1.2
#define Y_MIN -0.1
#define Y_MAX  0.1

int mandelbrot(double complex c) {
    double complex z = 0;
    int iter = 0;
    while (cabs(z) <= 2.0 && iter < MAX_ITER) {
        z = z * z + c;
        iter++;
    }
    return iter;
}

int main() {
    FILE *fp = fopen("mandelbrot_output.dat", "w");
    if (!fp) {
        perror("Erro ao abrir arquivo de saída");
        return 1;
    }

    clock_t start = clock();

    // Alocação da matriz 2D
    int **matriz = malloc(HEIGHT * sizeof(int *));
    if (!matriz) {
        perror("Erro de alocação de linhas");
        fclose(fp);
        return 1;
    }

    for (int i = 0; i < HEIGHT; i++) {
        matriz[i] = malloc(WIDTH * sizeof(int));
        if (!matriz[i]) {
            perror("Erro de alocação de colunas");
            // Libera linhas anteriores
            for (int j = 0; j < i; j++) free(matriz[j]);
            free(matriz);
            fclose(fp);
            return 1;
        }
    }

    // Calcula os valores
    for (int py = 0; py < HEIGHT; py++) {
        for (int px = 0; px < WIDTH; px++) {
            double x0 = X_MIN + px * (X_MAX - X_MIN) / (WIDTH - 1);
            double y0 = Y_MIN + py * (Y_MAX - Y_MIN) / (HEIGHT - 1);
            double complex c = x0 + y0 * I;
            matriz[py][px] = mandelbrot(c);
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    // Cabeçalho
    fprintf(fp, "%d %d %.4f %.17f %.17f %.17f %.17f\n", WIDTH, HEIGHT, elapsed, X_MIN, X_MAX, Y_MIN, Y_MAX);

    // Grava a matriz
    for (int py = 0; py < HEIGHT; py++) {
        for (int px = 0; px < WIDTH; px++) {
            fprintf(fp, "%d ", matriz[py][px]);
        }
        fprintf(fp, "\n");
    }

    // Libera memória
    for (int i = 0; i < HEIGHT; i++) {
        free(matriz[i]);
    }
    free(matriz);
    fclose(fp);

    printf("Arquivo mandelbrot_output.dat gerado com sucesso.\n");
    printf("Tempo de execução: %.4f segundos\n", elapsed);

    return 0;
}
