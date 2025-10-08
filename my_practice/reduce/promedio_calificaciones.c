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

    // Simular que cada proceso tiene calificaciones de algunos estudiantes
    int calificaciones_locales[3];
    int suma_local = 0;

    // Cada proceso genera calificaciones aleatorias (o podrían ser reales)
    srand(rank + 1);  // Semilla diferente para cada proceso
    for(int i = 0; i < 3; i++)
    {
        calificaciones_locales[i] = 60 + rand() % 40;  // Calificaciones entre 60-99
        suma_local += calificaciones_locales[i];
    }

    printf("Proceso %d: Calificaciones locales = [%d, %d, %d], Suma local = %d\n",
           rank, calificaciones_locales[0], calificaciones_locales[1],
           calificaciones_locales[2], suma_local);

    // ===========================================
    // USO DE MPI_REDUCE PARA CALCULAR SUMA TOTAL
    // ===========================================
    int suma_total;
    MPI_Reduce(&suma_local, &suma_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // ===========================================
    // TAMBIÉN NECESITAMOS LA CANTIDAD TOTAL
    // ===========================================
    int cantidad_local = 3;  // Cada proceso tiene 3 calificaciones
    int cantidad_total;
    MPI_Reduce(&cantidad_local, &cantidad_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(rank == 0)
    {
        double promedio = (double)suma_total / cantidad_total;

        printf("\n=== PROMEDIO DE CALIFICACIONES ===\n");
        printf("Suma total de todas las calificaciones: %d\n", suma_total);
        printf("Cantidad total de calificaciones: %d\n", cantidad_total);
        printf("Promedio general: %.2f\n", promedio);
    }

    MPI_Finalize();
    return 0;
}