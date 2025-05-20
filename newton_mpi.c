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
        // MESTRE
        int **matriz = malloc(ALTURA * sizeof(int *));
        for (int i = 0; i < ALTURA; i++)
            matriz[i] = malloc(LARGURA * sizeof(int));

        int linha_atual = 0, ativos = 0;
        double inicio = MPI_Wtime();

        // Distribui as primeiras tarefas
        for (int i = 1; i < size && linha_atual < ALTURA; i++) {
            MPI_Send(&linha_atual, 1, MPI_INT, i, TAG_TRABALHO, MPI_COMM_WORLD);
            linha_atual++;
            ativos++;
        }

        while (ativos > 0) {
            int linha_recebida;
            int origem;
            MPI_Status status;

            // Recebe linha processada
            int *linha_dados = malloc(LARGURA * sizeof(int));
            MPI_Recv(linha_dados, LARGURA, MPI_INT, MPI_ANY_SOURCE, TAG_RESULTADO, MPI_COMM_WORLD, &status);
            origem = status.MPI_SOURCE;
            MPI_Recv(&linha_recebida, 1, MPI_INT, origem, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Armazena na matriz
            for (int j = 0; j < LARGURA; j++)
                matriz[linha_recebida][j] = linha_dados[j];
            free(linha_dados);

            // Envia nova linha ou termina
            if (linha_atual < ALTURA) {
                MPI_Send(&linha_atual, 1, MPI_INT, origem, TAG_TRABALHO, MPI_COMM_WORLD);
                linha_atual++;
            } else {
                int fim = -1;
                MPI_Send(&fim, 1, MPI_INT, origem, TAG_TERMINO, MPI_COMM_WORLD);
                ativos--;
            }
        }

        double fim = MPI_Wtime();
        salvar_matriz(matriz, fim - inicio);

        for (int i = 0; i < ALTURA; i++) free(matriz[i]);
        free(matriz);

        printf("Fractal gerado com MPI em %.4f segundos\n", fim - inicio);
    } else {
        // ESCRAVO
        while (1) {
            int linha;
            MPI_Status status;
            MPI_Recv(&linha, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TAG_TERMINO) break;

            double y = Y_MIN + (Y_MAX - Y_MIN) * linha / (ALTURA - 1);
            int *linha_resultado = malloc(LARGURA * sizeof(int));

            for (int x = 0; x < LARGURA; x++) {
                double real = X_MIN + (X_MAX - X_MIN) * x / (LARGURA - 1);
                complex double z = real + y * I;
                linha_resultado[x] = calcula_convergencia(z);
            }

            MPI_Send(linha_resultado, LARGURA, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
            MPI_Send(&linha, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);

            free(linha_resultado);
        }
    }

    MPI_Finalize();
    return 0;
}
