//
// Created by cjorg on 10/7/2025.
// Ordenar un vector de tamaño arbitrario.
// Cada proceso recibe un bloque (posible irregular),
// lo ordena localmente y el root (rank 0)  hace un
// ordenamiento final para obtener el arreglo globalmente
// ordenado.
//


#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ===========================================
    // PARTE 1: PREPARACIÓN DE DATOS
    // ===========================================
    int *array_completo = NULL;
    int total_numeros = 8; // Podemos cambiar este número fácilmente

    // Solo el proceso 0 crea el arreglo completo(Inicializacion)
    if (rank == 0) {
        array_completo = (int *) malloc(total_numeros * sizeof(int));

        for (int i = 0; i < total_numeros; i++) {
            array_completo[i] = total_numeros - i;
        }

        printf("Proceso %d: Arreglo completo = ", rank);
        for (int i = 0; i < total_numeros; i++) {
            printf("%d ", array_completo[i]);
        }
        printf("\n");
    }
    // ===========================================
    // PARTE 2: CALCULAR LA DISTRIBUCIÓN
    // ===========================================
    // Necesitamos calcular cuántos elementos le tocan a cada proceso
    int *sendcounts = NULL;  // Cuántos elementos manda a cada proceso
    int *displs = NULL;      // Desplazamientos para cada proceso

    if (rank == 0) {
        sendcounts = (int *) malloc(size * sizeof(int));
        displs = (int *) malloc(size * sizeof(int));

        int base = total_numeros / size;     // Cantidad base para cada proceso
        int resto = total_numeros % size;    // Elementos sobrantes

        printf("Distribución: %d elementos, %d procesos\n", total_numeros, size);
        printf("Cada proceso recibe al menos %d elementos\n", base);
        printf("%d procesos reciben 1 elemento extra\n", resto);

        // Calcular sendcounts y displs
        int desplazamiento_actual = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = base;
            if (i < resto)  // Los primeros 'resto' procesos reciben un elemento extra
            {
                sendcounts[i]++;
            }
            displs[i] = desplazamiento_actual;
            desplazamiento_actual += sendcounts[i];

            printf("Proceso %d: recibe %d elementos, desplazamiento %d\n",
                   i, sendcounts[i], displs[i]);
        }
    }

    // Cada proceso necesita saber cuántos elementos va a recibir
    int mi_cantidad;

    // Distribuimos sendcounts para que cada proceso sepa cuántos elementos recibirá
    MPI_Scatter(sendcounts, 1, MPI_INT, &mi_cantidad, 1, MPI_INT, 0, MPI_COMM_WORLD);

    printf("Proceso %d: Voy a recibir %d elementos\n", rank, mi_cantidad);

    // Cada proceso crea un arreglo para recibir SU parte
    int *mi_parte = (int *) malloc(mi_cantidad * sizeof(int));


    // ===========================================
    // PARTE 3: DISTRIBUCIÓN CON MPI_SCATTERV
    // ===========================================
    MPI_Scatterv(
            array_completo,      // Arreglo que se va a dividir
            sendcounts,          // Cuántos elementos para CADA proceso
            displs,              // Desplazamiento para CADA proceso
            MPI_INT,             // Tipo de dato
            mi_parte,            // Donde cada proceso recibe SU parte
            mi_cantidad,         // Cuántos elementos recibe ESTE proceso
            MPI_INT,             // Tipo de dato
            0,                   // Proceso raíz
            MPI_COMM_WORLD
    );
    // ===========================================
    // PARTE 4: ELEVAR AL CUADRADO (Logica del programa)
    // ===========================================


    printf("Proceso %d: Recibí -> ", rank);
    for (int i = 0; i < mi_cantidad; i++) {
        printf("%d ", mi_parte[i]);
    }
    printf("\n");


    // Cada proceso ordena sus numeros
    int temp = -1;
    printf("Proceso %d: Ordenando -> ", rank);
    for (int i = 0; i < mi_cantidad - 1; i++) {
        for (int j = i + 1; j < mi_cantidad; j++) {
            if (mi_parte[i] > mi_parte[j]) {
                temp = mi_parte[i];
                mi_parte[i] = mi_parte[j];
                mi_parte[j] = temp;
            }
        }
    }
    printf("Proceso %d mi parte ordenada: -> ", rank);
    printf("\n");
    //Mostrar parte ordenada
    for (int i = 0; i < mi_cantidad; i++) {
        printf(" %d - ", mi_parte[i]);
    }

    // ===========================================
    // PARTE 5: RECOLECCIÓN CON MPI_GATHERV
    // ===========================================
    int *resultados_completos = NULL;
    int *recvcounts = NULL;  // Para Gatherv también necesitamos recvcounts
    int *rdispls = NULL;     // y desplazamientos de recepción

    if (rank == 0) {
        resultados_completos = (int *) malloc(total_numeros * sizeof(int));//Cantidad de elementos del arreglo
        recvcounts = (int *) malloc(size * sizeof(int));
        rdispls = (int *) malloc(size * sizeof(int));

        // Para Gatherv, recvcounts es igual a sendcounts
        // y rdispls es igual a displs
        for (int i = 0; i < size; i++) {
            recvcounts[i] = sendcounts[i];
            rdispls[i] = displs[i];
        }
    }
    MPI_Gatherv(
            mi_parte,                // Lo que cada proceso envía
            mi_cantidad,             // Cuántos elementos envía ESTE proceso
            MPI_INT,                 // Tipo de dato
            resultados_completos,    // Donde se reunen todos
            recvcounts,              // Cuántos elementos recibe de CADA proceso
            rdispls,                 // Desplazamientos para CADA proceso
            MPI_INT,                 // Tipo de dato
            0,                       // Proceso destino
            MPI_COMM_WORLD
    );

    // El proceso 0 muestra los resultados finales
    if (rank == 0) {
        printf("\n=== RESULTADOS FINALES ===\n");
        printf("Todos los cuadrados calculados: ");
        for (int i = 0; i < total_numeros; i++) {
            printf("%d ", resultados_completos[i]);
        }
        printf("\n");

        // Liberar memoria
        free(resultados_completos);
        free(array_completo);
        free(sendcounts);
        free(displs);
        free(recvcounts);
        free(rdispls);
    }

    free(mi_parte);

    MPI_Finalize();
    return 0;
}