//
// Created by cjorg on 10/8/2025.
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

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ===========================================
    // PARTE 1: INICIALIZACIÓN Y CONFIGURACIÓN
    // ===========================================
    int *vector_completo = NULL;
    int total_elementos = 0;

    if(rank == 0) {
        // Configurar el tamaño del vector
        total_elementos = 100;  // Podemos cambiar este valor

        // Crear y llenar el vector con valores aleatorios
        vector_completo = (int*)malloc(total_elementos * sizeof(int));
        srand(time(NULL));

        printf("=== ORDENAMIENTO DISTRIBUIDO DE VECTOR ===\n");
        printf("Tamaño del vector: %d elementos\n", total_elementos);
        printf("Número de procesos: %d\n", size);

        printf("Vector original: ");
        for(int i = 0; i < total_elementos; i++) {
            vector_completo[i] = rand() % 1000;  // Valores entre 0-999
            printf("%d ", vector_completo[i]);
        }
        printf("\n\n");
    }

    // Broadcast del tamaño total a todos los procesos
    MPI_Bcast(&total_elementos, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // ===========================================
    // PARTE 2: DISTRIBUCIÓN DEL VECTOR
    // ===========================================
    int *sendcounts = NULL;
    int *displs = NULL;

    if(rank == 0) {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));

        // Calcular distribución de elementos
        int base = total_elementos / size;
        int resto = total_elementos % size;

        printf("Distribución de elementos:\n");
        int desplazamiento_actual = 0;
        for(int i = 0; i < size; i++) {
            sendcounts[i] = base;
            if(i < resto) {
                sendcounts[i]++;  // Los primeros procesos reciben un elemento extra
            }
            displs[i] = desplazamiento_actual;
            desplazamiento_actual += sendcounts[i];

            printf("  Proceso %d: %d elementos (desplazamiento: %d)\n",
                   i, sendcounts[i], displs[i]);
        }
        printf("\n");
    }

    // Cada proceso descubre cuántos elementos recibirá
    int mi_cantidad;
    MPI_Scatter(sendcounts, 1, MPI_INT, &mi_cantidad, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso prepara buffer para recibir su segmento
    int *mi_segmento = (int*)malloc(mi_cantidad * sizeof(int));

    // Distribuir los segmentos del vector
    MPI_Scatterv(
            vector_completo, sendcounts, displs, MPI_INT,
            mi_segmento, mi_cantidad, MPI_INT,
            0, MPI_COMM_WORLD
    );

    // ===========================================
    // PARTE 3: ORDENAMIENTO LOCAL
    // ===========================================
    printf("Proceso %d: Recibí %d elementos -> ", rank, mi_cantidad);
    for(int i = 0; i < mi_cantidad; i++) {
        printf("%d ", mi_segmento[i]);
    }
    printf("\n");

    // Ordenar el segmento local usando qsort
    qsort(mi_segmento, mi_cantidad, sizeof(int), comparar_enteros);

    printf("Proceso %d: Ordenado localmente -> ", rank);
    for(int i = 0; i < mi_cantidad; i++) {
        printf("%d ", mi_segmento[i]);
    }
    printf("\n");

    // ===========================================
    // PARTE 4: RECOLECCIÓN Y ORDENAMIENTO FINAL
    // ===========================================
    int *vector_ordenado = NULL;
    int *recvcounts = NULL;
    int *rdispls = NULL;

    if(rank == 0) {
        vector_ordenado = (int*)malloc(total_elementos * sizeof(int));
        recvcounts = (int*)malloc(size * sizeof(int));
        rdispls = (int*)malloc(size * sizeof(int));

        // Para Gatherv, recvcounts y rdispls son iguales a sendcounts y displs
        for(int i = 0; i < size; i++) {
            recvcounts[i] = sendcounts[i];
            rdispls[i] = displs[i];
        }
    }

    // Recolectar todos los segmentos ordenados
    MPI_Gatherv(
            mi_segmento, mi_cantidad, MPI_INT,
            vector_ordenado, recvcounts, rdispls, MPI_INT,
            0, MPI_COMM_WORLD
    );

    // ===========================================
    // PARTE 5: MEZCLA FINAL (solo en proceso 0)
    // ===========================================
    if(rank == 0) {
        // Preparar arrays para la mezcla
        int **arrays_ordenados = (int**)malloc(size * sizeof(int*));
        int *tamanos_arrays = (int*)malloc(size * sizeof(int));

        for(int i = 0; i < size; i++) {
            arrays_ordenados[i] = vector_ordenado + rdispls[i];
            tamanos_arrays[i] = recvcounts[i];
        }

        // Crear vector temporal para el resultado de la mezcla
        int *vector_mezclado = (int*)malloc(total_elementos * sizeof(int));

        // Mezclar todos los segmentos ordenados - CORREGIDO: pasar total_elementos
        mezclar_arrays_ordenados(arrays_ordenados, tamanos_arrays, size, vector_mezclado, total_elementos);

        // Copiar el resultado mezclado al vector final
        for(int i = 0; i < total_elementos; i++) {
            vector_ordenado[i] = vector_mezclado[i];
        }

        free(vector_mezclado);
        free(arrays_ordenados);
        free(tamanos_arrays);

        // Mostrar resultados
        printf("\n=== RESULTADO FINAL ===\n");
        printf("Vector completamente ordenado: ");
        for(int i = 0; i < total_elementos; i++) {
            printf("%d ", vector_ordenado[i]);
        }
        printf("\n");

        // Verificar que está ordenado
        int ordenado = 1;
        for(int i = 1; i < total_elementos; i++) {
            if(vector_ordenado[i] < vector_ordenado[i-1]) {
                ordenado = 0;
                break;
            }
        }
        printf("Verificación: El vector %s está correctamente ordenado.\n",
               ordenado ? "SÍ" : "NO");

        // Liberar memoria
        free(vector_completo);
        free(vector_ordenado);
        free(sendcounts);
        free(displs);
        free(recvcounts);
        free(rdispls);
    }

    // ===========================================
    // PARTE 6: LIMPIEZA
    // ===========================================
    free(mi_segmento);

    MPI_Finalize();
    return 0;
}