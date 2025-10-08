//
// Created by cjorg on 10/7/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int mi_valor = (rank + 1) * 10;  // 10, 20, 30, 40...

    int suma, producto, maximo, minimo;

    // Diferentes operaciones de reducción
    MPI_Reduce(&mi_valor, &suma, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&mi_valor, &producto, 1, MPI_INT, MPI_PROD, 0, MPI_COMM_WORLD);
    MPI_Reduce(&mi_valor, &maximo, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&mi_valor, &minimo, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    if(rank == 0)
    {
        printf("=== RESULTADOS CON DIFERENTES OPERACIONES ===\n");
        printf("Valores individuales: ");
        for(int i = 0; i < size; i++)
        {
            printf("%d ", (i + 1) * 10);
        }
        printf("\n");

        printf("SUMA: %d\n", suma);
        printf("PRODUCTO: %d\n", producto);
        printf("MÁXIMO: %d\n", maximo);
        printf("MÍNIMO: %d\n", minimo);
    }

    MPI_Finalize();
    return 0;
}