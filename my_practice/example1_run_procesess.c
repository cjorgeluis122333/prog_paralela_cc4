//
// Created by cjorg on 10/5/2025.
//
#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hola desde el proceso %d de %d\n", rank, size);

    MPI_Finalize();
    return 0;
}
