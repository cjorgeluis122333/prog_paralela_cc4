#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/* Reparte N en P bloques (posible irregular) */
static void build_counts_displs(int N, int P, int *counts, int *displs) {
    int base = N / P, rem = N % P, off = 0;
    for (int i = 0; i < P; ++i) {
        counts[i] = base + (i < rem ? 1 : 0);
        displs[i] = off;
        off += counts[i];
    }
}

/* Ordenamiento por inserción (ascendente) */
static void insertion_sort(int *a, int n) {
    for (int i = 1; i < n; ++i) {
        int key = a[i], j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j+1] = a[j];
            --j;
        }
        a[j+1] = key;
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size; 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Tamaño total (opcional por argv), por defecto 20 */
    int N = (argc > 1) ? atoi(argv[1]) : 20;

    /* Root crea los datos: descendente para que se note el ordenado */
    int *A = NULL;
    if (rank == 0) {
        A = (int*)malloc(N * sizeof(int));
        for (int i = 0; i < N; ++i) A[i] = N - i;  // N, N-1, ..., 1
    }

    /* counts/displs iguales en todos (no hace falta Bcast si todos lo calculan) */
    int *counts = (int*)malloc(size * sizeof(int));
    int *displs = (int*)malloc(size * sizeof(int));
    build_counts_displs(N, size, counts, displs);

    int local_n = counts[rank];
    int *local  = (int*)malloc((local_n > 0 ? local_n : 1) * sizeof(int));

    /* Repartir bloques irregulares */
    MPI_Scatterv(A, counts, displs, MPI_INT,
                 local, local_n, MPI_INT,
                 0, MPI_COMM_WORLD);

    /* Orden local (no bloqueante de cómputo con otros procesos) */
    if (local_n > 0) insertion_sort(local, local_n);

    /* Recolectar en el root */
    MPI_Gatherv(local, local_n, MPI_INT,
                A, counts, displs, MPI_INT,
                0, MPI_COMM_WORLD);

    /* orden final completo en el root, también con insertion_sort */
    if (rank == 0) {
        insertion_sort(A, N);
        printf("Sorted array (%d elems):", N);
        for (int i = 0; i < N; ++i) printf(" %d", A[i]);
        printf("\n");
        free(A);
    }

    free(local);
    free(counts);
    free(displs);
    MPI_Finalize();
    return 0;
}
