// scatterv_gatherv_square.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

static void build_counts_displs(int N, int P, int *counts, int *displs) {
    int base = N / P, rem = N % P;
    int off = 0;
    for (int i = 0; i < P; ++i) {
        counts[i] = base + (i < rem ? 1 : 0);
        displs[i] = off;
        off += counts[i];
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size; 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 27;
    int *A = NULL, *B = NULL;
    if (rank == 0) {
        A = (int*)malloc(N*sizeof(int));
        B = (int*)malloc(N*sizeof(int));
        for (int i = 0; i < N; ++i) A[i] = i+1; // 1..N
    }

    int *counts = (int*)malloc(size*sizeof(int));
    int *displs = (int*)malloc(size*sizeof(int));
    if (rank == 0) build_counts_displs(N, size, counts, displs);
    // Broadcast de counts/displs para que todos conozcan su tamaño local
    MPI_Bcast(counts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

    int local_n = counts[rank];
    int *local = (int*)malloc((local_n>0?local_n:1)*sizeof(int));

    MPI_Scatterv(A, counts, displs, MPI_INT,
                 local, local_n, MPI_INT,
                 0, MPI_COMM_WORLD);

    for (int i = 0; i < local_n; ++i) local[i] = local[i]*local[i];

    MPI_Gatherv(local, local_n, MPI_INT,
                B, counts, displs, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("B =");
        for (int i = 0; i < N; ++i) printf(" %d", B[i]);
        printf("\n");
    }

    free(local); free(counts); free(displs);
    if (rank == 0) { free(A); free(B); }
    MPI_Finalize();
    return 0;
