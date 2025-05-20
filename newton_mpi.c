#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

#define LARGURA 4000
#define ALTURA 4000
#define MAX_ITERACOES 1000
#define EPSILON 1e-6

#define X_MIN -0.05
#define X_MAX  0.05
#define Y_MIN -0.05
#define Y_MAX  0.05

#define TAG_TRABALHO 1
#define TAG_RESULTADO 2
#define TAG_TERMINO 3

int calcula_convergencia(complex double z) {
    for (int i = 0; i < MAX_ITERACOES; i++) {
        complex double f = cpow(z, 3) - 1;
        complex double f_linha = 3 * cpow(z, 2);
        if (cabs(f) < EPSILON) return i;
        z = z - f / f_linha;
    }
    return MAX_ITERACOES;
}

void salvar_matriz(int **matriz, double tempo_execucao) {
    FILE *fp = fopen("newton_mpi_output.dat", "w");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }
    fprintf(fp, "%d %d %.4f %.17f %.17f %.17f %.17f\n",
            LARGURA, ALTURA, tempo_execucao, X_MIN, X_MAX, Y_MIN, Y_MAX);
            
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            fprintf(fp, "%d", matriz[y][x]);
            if (x < LARGURA - 1) fprintf(fp, " ");
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        int **matriz = malloc(ALTURA * sizeof(int *));
        for (int i = 0; i < ALTURA; i++)
            matriz[i] = malloc(LARGURA * sizeof(int));

        int linha_atual = 0, ativos = 0;
        double inicio = MPI_Wtime();

        for (int i = 1; i < size && linha_atual < ALTURA; i++) {
            complex double *linha_z = malloc((LARGURA + 1) * sizeof(complex double));
            linha_z[0] = linha_atual + 0.0 * I;
            double y = Y_MIN + (Y_MAX - Y_MIN) * linha_atual / (ALTURA - 1);
            for (int x = 0; x < LARGURA; x++) {
                double real = X_MIN + (X_MAX - X_MIN) * x / (LARGURA - 1);
                linha_z[1 + x] = real + y * I;
            }
            MPI_Send(linha_z, LARGURA + 1, MPI_C_DOUBLE_COMPLEX, i, TAG_TRABALHO, MPI_COMM_WORLD);
            free(linha_z);
            linha_atual++;
            ativos++;
        }

        while (ativos > 0) {
            int *resultado = malloc((LARGURA + 1) * sizeof(int));
            MPI_Status status;
            MPI_Recv(resultado, LARGURA + 1, MPI_INT, MPI_ANY_SOURCE, TAG_RESULTADO, MPI_COMM_WORLD, &status);

            int linha_id = resultado[0];
            for (int j = 0; j < LARGURA; j++)
                matriz[linha_id][j] = resultado[j + 1];

            free(resultado);

            if (linha_atual < ALTURA) {
                complex double *linha_z = malloc((LARGURA + 1) * sizeof(complex double));
                linha_z[0] = linha_atual + 0.0 * I;
                double y = Y_MIN + (Y_MAX - Y_MIN) * linha_atual / (ALTURA - 1);
                for (int x = 0; x < LARGURA; x++) {
                    double real = X_MIN + (X_MAX - X_MIN) * x / (LARGURA - 1);
                    linha_z[1 + x] = real + y * I;
                }
                MPI_Send(linha_z, LARGURA + 1, MPI_C_DOUBLE_COMPLEX, status.MPI_SOURCE, TAG_TRABALHO, MPI_COMM_WORLD);
                free(linha_z);
                linha_atual++;
            } else {
                MPI_Send(NULL, 0, MPI_C_DOUBLE_COMPLEX, status.MPI_SOURCE, TAG_TERMINO, MPI_COMM_WORLD);
                ativos--;
            }
        }

        double fim = MPI_Wtime();
        salvar_matriz(matriz, fim - inicio);
        for (int i = 0; i < ALTURA; i++) free(matriz[i]);
        free(matriz);

        printf("Fractal gerado com MPI (z pronto) em %.4f segundos\n", fim - inicio);
    } else {
        while (1) {
            complex double *linha_z = malloc((LARGURA + 1) * sizeof(complex double));
            MPI_Status status;

            MPI_Recv(linha_z, LARGURA + 1, MPI_C_DOUBLE_COMPLEX, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == TAG_TERMINO) {
                free(linha_z);
                break;
            }

            int linha_id = (int)creal(linha_z[0]);
            int *resultado = malloc((LARGURA + 1) * sizeof(int));
            resultado[0] = linha_id;

            for (int i = 0; i < LARGURA; i++) {
                resultado[i + 1] = calcula_convergencia(linha_z[i + 1]);
            }

            MPI_Send(resultado, LARGURA + 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
            free(linha_z);
            free(resultado);
        }
    }

    MPI_Finalize();
    return 0;
}
