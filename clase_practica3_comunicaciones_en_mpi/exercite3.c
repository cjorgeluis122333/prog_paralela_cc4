//
// Created by cjorg on 9/24/2025.
//
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
// mpicc calculate_total_sum.c -o calculate_total_sum
// mpirun -np 3 ./calculate_total_sum
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Largo del vector (opcional por argv) */
    int N = (argc > 1) ? atoi(argv[1]) : 8;

    double *local = (double*)malloc(N * sizeof(double));
    double *sum   = (double*)malloc(N * sizeof(double));
    double *vmax  = (double*)malloc(N * sizeof(double));

    /* Cada proceso arma su vector local (ejemplo simple) */
    for (int i = 0; i < N; ++i) {
        local[i] = rank * 10.0 + i;   // p.ej., rank=2 -> 20,21,22,...
    }

    // Allreduce for recolet and join the multiple process and recolet the results
    MPI_Allreduce(local, sum,  N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(local, vmax, N, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Vector SUM (N=%d):\n", N);
        for (int i = 0; i < N; ++i) printf("%.0f ", sum[i]);
        printf("\n\nVector MAX:\n");
        for (int i = 0; i < N; ++i) printf("%.0f ", vmax[i]);
        printf("\n\nVector AVG (SUM/size):\n");
        for (int i = 0; i < N; ++i) printf("%.2f ", sum[i] / size);
        printf("\n");
    }

    free(local); free(sum); free(vmax);
    MPI_Finalize();
    return 0;
}
