#include <mpi.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// mpicc calculate_total_sum.c -o calculate_total_sum
// mpirun -np 3 ./calculate_total_sum
static void make_counts_displs_by_ratio(int N, int P, int *counts, int *displs) {
    // ejemplo: repartir proporcional a (i+1) para que sean irregulares
    int sumw = P*(P+1)/2; // 1+2+...+P
    int off = 0, acc = 0;
    for (int i = 0; i < P; ++i) {
        // asigna un tramo aproximado N*(i+1)/sumw (ajuste para sumar N)
        int next = (int)((long long)N * (i+1) / sumw);
        counts[i] = next;
        acc += next;
    }
    // corrige por posibles redondeos
    counts[P-1] += (N - acc);
    for (int i = 0; i < P; ++i) { displs[i] = off; off += counts[i]; }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size; 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char *text = NULL;
    int N = 0;
    if (rank == 0) {
        const char *msg = "Scatterv/Gatherv con texto de longitud variable en MPI.";
        N = (int)strlen(msg);
        text = (char*)malloc(N*sizeof(char));
        memcpy(text, msg, N);
    }
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int *counts = (int*)malloc(size*sizeof(int));
    int *displs = (int*)malloc(size*sizeof(int));
    if (rank == 0) make_counts_displs_by_ratio(N, size, counts, displs);
    MPI_Bcast(counts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

    int local_n = counts[rank];
    char *local = (char*)malloc((local_n>0?local_n:1)*sizeof(char));

    MPI_Scatterv(text, counts, displs, MPI_CHAR,
                 local, local_n, MPI_CHAR,
                 0, MPI_COMM_WORLD);

    for (int i = 0; i < local_n; ++i) local[i] = (char)toupper((unsigned char)local[i]);

    char *out = NULL;
    if (rank == 0) out = (char*)malloc(N*sizeof(char));

    MPI_Gatherv(local, local_n, MPI_CHAR,
                out, counts, displs, MPI_CHAR,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("OUT: %.*s\n", N, out);
        free(out); free(text);
    }
    free(local); free(counts); free(displs);
    MPI_Finalize();
    return 0;
}
