// Simple: Cálculo de la Suma Total con MPI_Reduce
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

    // ===========================================
    // PARTE 1: CADA PROCESO TIENE SUS DATOS LOCALES
    // ===========================================
    int mi_numero_local;

    // Cada proceso genera su propio número basado en su rank
    // Esto simula que cada proceso hizo algún cálculo local
    mi_numero_local = (rank + 1) * 10;  // Proceso 0: 10, Proceso 1: 20, etc.

    printf("Proceso %d: Mi número local es %d\n", rank, mi_numero_local);

    // ===========================================
    // PARTE 2: MPI_REDUCE - SUMAR TODOS LOS NÚMEROS
    // ===========================================
    int suma_total;

    MPI_Reduce(
            &mi_numero_local,    // Lo que cada proceso contribuye
            &suma_total,         // Donde se guarda el resultado (solo proceso 0)
            1,                   // Cantidad de elementos
            MPI_INT,             // Tipo de dato
            MPI_SUM,             // Operación: SUMA
            0,                   // Proceso que recibe el resultado
            MPI_COMM_WORLD       // Comunicador
    );

    // ===========================================
    // PARTE 3: MOSTRAR RESULTADOS
    // ===========================================
    if(rank == 0)
    {
        printf("\n=== RESULTADO DE MPI_REDUCE ===\n");
        printf("La suma total de todos los procesos es: %d\n", suma_total);

        // Verificación manual (opcional)
        int verificacion = 0;
        for(int i = 0; i < size; i++)
        {
            verificacion += (i + 1) * 10;
        }
        printf("Verificación manual: %d\n", verificacion);
    }

    MPI_Finalize();
    return 0;
}