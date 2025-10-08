//
// Created by cjorg on 10/6/2025.
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
    // PARTE 1: PREPARACIÓN DE DATOS (Solo proceso 0)
    // ===========================================
    int *array_completo = NULL;
    int total_numeros = 8; // Vamos a usar 8 números

    // Solo el proceso 0 crea el arreglo completo
    if(rank == 0)
    {
        array_completo = (int*)malloc(total_numeros * sizeof(int));

        // Llenamos el arreglo con valores
        for(int i = 0; i < total_numeros; i++){
            array_completo[i] = i + 1; // [1, 2, 3, 4, 5, 6, 7, 8]
        }

        printf("Proceso %d: Arreglo completo = ", rank);
        for(int i = 0; i < total_numeros; i++){
            printf("%d ", array_completo[i]);
        }
        printf("\n");
    }

    // ===========================================
    // PARTE 2: DISTRIBUCIÓN CON MPI_SCATTER
    // ===========================================
    // Calculamos cuántos números le tocan a cada proceso
    int numeros_por_proceso = total_numeros / size;
    // Cada proceso crea un arreglo pequeño para recibir SU parte
    int *mi_parte = (int*)malloc(numeros_por_proceso * sizeof(int));

    printf("Proceso %d: Esperando recibir %d números...\n", rank, numeros_por_proceso);

    // MPI_Scatter DIVIDE el arreglo grande y envía partes a cada proceso
    MPI_Scatter(
            array_completo,          // Arreglo que se va a dividir (solo lo tiene el proceso 0)
            numeros_por_proceso,     // Cuántos elementos envía a CADA proceso
            MPI_INT,                 // Tipo de dato
            mi_parte,                // Donde cada proceso recibe SU parte
            numeros_por_proceso,     // Cuántos elementos recibe CADA proceso
            MPI_INT,                 // Tipo de dato
            0,                       // Proceso raíz (que tiene los datos originales)
            MPI_COMM_WORLD           // Comunicador
    );

    // ===========================================
    // PARTE 3: CADA PROCESO TRABAJA CON SUS DATOS
    // ===========================================
    printf("Proceso %d: Recibí -> ", rank);
    for(int i = 0; i < numeros_por_proceso; i++){
        printf("%d ", mi_parte[i]);
    }
    printf("\n");

    // Cada proceso calcula los cuadrados de SUS números
    printf("Proceso %d: Calculando cuadrados -> ", rank);
    for(int i = 0; i < numeros_por_proceso; i++){
        mi_parte[i] = mi_parte[i] * mi_parte[i]; // Elevar al cuadrado
        printf("%d ", mi_parte[i]);
    }
    printf("\n");

    // ===========================================
    // PARTE 4: RECOLECCIÓN CON MPI_GATHER (OPCIONAL)
    // ===========================================
    int *resultados_completos = NULL;
    if(rank == 0)
    {
        resultados_completos = (int*)malloc(total_numeros * sizeof(int));
    }

    // MPI_Gather reúne todas las partes en el proceso 0
    MPI_Gather(
            mi_parte,                // Lo que cada proceso envía(Lista de Datos o Datos)
            numeros_por_proceso,     // Cuántos elementos envía CADA proceso
            MPI_INT,                 // Tipo de dato
            resultados_completos,    // Donde se reunen todos (solo proceso 0)
            numeros_por_proceso,     // Cuántos elementos recibe de CADA proceso
            MPI_INT,                 // Tipo de dato
            0,                       // Proceso destino
            MPI_COMM_WORLD           // Comunicador
    );

    // El proceso 0 muestra los resultados finales
    if(rank == 0)
    {
        printf("\n=== RESULTADOS FINALES ===\n");
        printf("Todos los cuadrados calculados: ");
        for(int i = 0; i < total_numeros; i++)
        {
            printf("%d ", resultados_completos[i]);
        }
        printf("\n");

        free(resultados_completos);
        free(array_completo);
    }

    free(mi_parte);
    MPI_Finalize();
    return 0;
}