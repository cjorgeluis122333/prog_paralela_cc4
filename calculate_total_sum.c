//
// mpicc calculate_total_sum.c -o calculate_total_sum
// mpirun -np 3 ./calculate_total_sum
//
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#define MCW MPI_COMM_WORLD

int main(int argc, char *argv[]) {
    int np, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MCW, &np);
    MPI_Comm_rank(MCW, &p);

    int N = 12;  // tamaño del vector (ejemplo). Debe ser divisible por np
    int *vector = NULL;
    int local_size = N / np;
    int *sub_vector = (int *)malloc(local_size * sizeof(int));

    // El root inicializa el vector
    if (p == 0) {
        vector = (int *)malloc(N * sizeof(int));
        for (int i = 0; i < N; i++) {
            vector[i] = i + 1;  // ejemplo: llena con 1,2,3,...
        }
        printf("Vector inicial: ");
        for (int i = 0; i < N; i++) {
            printf("%d ", vector[i]);
        }
        printf("\n");
    }

    // Distribuye el vector en partes iguales a cada proceso
    MPI_Scatter(vector, local_size, MPI_INT,
                sub_vector, local_size, MPI_INT,
                0, MCW);

    // Cada proceso calcula la suma de su bloque
    int local_sum = 0;
    for (int i = 0; i < local_size; i++) {
        local_sum += sub_vector[i];
    }
    printf("Proceso %d calculó suma parcial = %d\n", p, local_sum);

    // Recolecta las sumas parciales en el root
    int total_sum = 0;
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_INT, MPI_SUM, 0, MCW);

    // El root imprime la suma final
    if (p == 0) {
        printf("\nLa suma total del vector es: %d\n", total_sum);
        free(vector);
    }

    free(sub_vector);
    MPI_Finalize();
    return 0;
}
