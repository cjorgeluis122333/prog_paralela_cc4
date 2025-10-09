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
#include <time.h>
#include <limits.h>  // Para INT_MAX

// Función para comparar enteros (usada en qsort)
int comparar_enteros(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Función para mezclar múltiples arrays ordenados - CORREGIDA
void mezclar_arrays_ordenados(int **arrays, int *tamanos, int num_arrays, int *resultado, int total_elementos) {
    // Array para llevar el índice actual de cada array
    int *indices = (int*)calloc(num_arrays, sizeof(int));

    // Array para los valores actuales de cada array
    int *valores_actuales = (int*)malloc(num_arrays * sizeof(int));

    // Inicializar valores actuales
    for(int i = 0; i < num_arrays; i++) {
        if(tamanos[i] > 0) {
            valores_actuales[i] = arrays[i][0];
        } else {
            valores_actuales[i] = INT_MAX; // Valor máximo si el array está vacío
        }
    }

    // Mezclar los arrays - CORREGIDO: usar total_elementos
    for(int pos = 0; pos < total_elementos; pos++) {
        // Encontrar el valor mínimo entre los valores actuales
        int min_val = INT_MAX;
        int min_idx = -1;

        for(int i = 0; i < num_arrays; i++) {
            if(indices[i] < tamanos[i] && valores_actuales[i] < min_val) {
                min_val = valores_actuales[i];
                min_idx = i;
            }
        }

        // Si no encontramos mínimo, terminamos
        if(min_idx == -1) break;

        // Colocar el mínimo en el resultado
        resultado[pos] = min_val;

        // Avanzar en el array del mínimo
        indices[min_idx]++;
        if(indices[min_idx] < tamanos[min_idx]) {
            valores_actuales[min_idx] = arrays[min_idx][indices[min_idx]];
        } else {
            valores_actuales[min_idx] = INT_MAX;
        }
    }

    free(indices);
    free(valores_actuales);
}

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
        // Preparar arrays para la mezcla
        int **arrays_ordenados = (int**)malloc(size * sizeof(int*));
        int *tamanos_arrays = (int*)malloc(size * sizeof(int));

        for(int i = 0; i < size; i++) {
            arrays_ordenados[i] = resultados_completos + rdispls[i];
            tamanos_arrays[i] = recvcounts[i];
        }

        // Crear vector temporal para el resultado de la mezcla
        int *vector_mezclado = (int*)malloc(total_numeros * sizeof(int));

        // Mezclar todos los segmentos ordenados
        mezclar_arrays_ordenados(arrays_ordenados, tamanos_arrays, size, vector_mezclado, total_numeros);

        // Copiar el resultado mezclado al vector final
        for(int i = 0; i < total_numeros; i++) {
            resultados_completos[i] = vector_mezclado[i];
        }

        free(vector_mezclado);
        free(arrays_ordenados);
        free(tamanos_arrays);

        // Mostrar resultados
        printf("\n=== RESULTADO FINAL ===\n");
        printf("Vector completamente ordenado: ");
        for(int i = 0; i < total_numeros; i++) {
            printf("%d ", resultados_completos[i]);
        }
        printf("\n");

        // Verificar que está ordenado
        int ordenado = 1;
        for(int i = 1; i < total_numeros; i++) {
            if(resultados_completos[i] < resultados_completos[i-1]) {
                ordenado = 0;
                break;
            }
        }
        printf("Verificación: El vector %s está correctamente ordenado.\n",
               ordenado ? "SÍ" : "NO");


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