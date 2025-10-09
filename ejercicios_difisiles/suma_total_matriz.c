//
// Created by cjorg on 10/9/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

/**
 * Función para imprimir una matriz (solo para debugging)
 */
void imprimir_matriz(int *matriz, int filas, int columnas, int rank) {
    printf("Proceso %d - Matriz %dx%d:\n", rank, filas, columnas);
    for(int i = 0; i < filas; i++) {
        for(int j = 0; j < columnas; j++) {
            printf("%4d ", matriz[i * columnas + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * Función para calcular la suma de una submatriz
 */
int sumar_submatriz(int *submatriz, int filas, int columnas) {
    int suma = 0;
    for(int i = 0; i < filas; i++) {
        for(int j = 0; j < columnas; j++) {
            suma += submatriz[i * columnas + j];
        }
    }
    return suma;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ===========================================
    // PARTE 1: LECTURA DE LAS DIMENSIONES DE LA MATRIZ
    // ===========================================
    int M = 0, N = 0;  // Dimensiones de la matriz (M filas x N columnas)

    if(rank == 0) {
        printf("=== SUMA TOTAL DISTRIBUIDA DE MATRIZ ===\n");

        if(argc > 2) {
            // Si se proporciona como argumento
            M = atoi(argv[1]);
            N = atoi(argv[2]);
            printf("Dimensiones de matriz leídas de argumentos: %dx%d\n", M, N);
        } else {
            // Solicitar por entrada estándar
            printf("Ingrese el número de filas de la matriz (M): ");
            scanf("%d", &M);
            printf("Ingrese el número de columnas de la matriz (N): ");
            scanf("%d", &N);
            printf("Matriz de tamaño: %dx%d\n", M, N);
        }

        // Validar que M y N sean positivos
        if(M <= 0 || N <= 0) {
            printf("Error: Las dimensiones de la matriz deben ser positivas.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Número de procesos: %d\n\n", size);
    }

    // Distribuir las dimensiones M y N a todos los procesos
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Validar que la matriz pueda ser distribuida
    if(M == 0 || N == 0) {
        MPI_Finalize();
        return 0;
    }

    // ===========================================
    // PARTE 2: CREACIÓN Y LLENADO DE LA MATRIZ
    // ===========================================
    int *matriz_completa = NULL;

    if(rank == 0) {
        // Crear matriz M x N
        matriz_completa = (int*)malloc(M * N * sizeof(int));

        // Llenar con valores aleatorios (0-99)
        srand(time(NULL));
        printf("Matriz completa (%dx%d):\n", M, N);
        for(int i = 0; i < M; i++) {
            for(int j = 0; j < N; j++) {
                matriz_completa[i * N + j] = rand() % 100;
                printf("%4d ", matriz_completa[i * N + j]);
            }
            printf("\n");
        }
        printf("\n");
    }

    // ===========================================
    // PARTE 3: DISTRIBUCIÓN DE FILAS ENTRE PROCESOS
    // ===========================================
    // Calcular cuántas filas le tocan a cada proceso
    int *filas_por_proceso = NULL;
    int *sendcounts = NULL;  // Número de ELEMENTOS por proceso
    int *desplazamientos = NULL;

    if(rank == 0) {
        filas_por_proceso = (int*)malloc(size * sizeof(int));
        sendcounts = (int*)malloc(size * sizeof(int));
        desplazamientos = (int*)malloc(size * sizeof(int));

        int filas_base = M / size;
        int filas_extra = M % size;
        int desplazamiento_actual = 0;

        printf("Distribución de filas:\n");
        for(int i = 0; i < size; i++) {
            filas_por_proceso[i] = filas_base;
            if(i < filas_extra) {
                filas_por_proceso[i]++;  // Procesos iniciales reciben fila extra
            }
            sendcounts[i] = filas_por_proceso[i] * N;  // Número de elementos
            desplazamientos[i] = desplazamiento_actual * N;  // En elementos
            desplazamiento_actual += filas_por_proceso[i];

            printf("  Proceso %d: %d filas (%d elementos, desplazamiento: %d)\n",
                   i, filas_por_proceso[i], sendcounts[i], desplazamientos[i]);
        }
        printf("\n");
    }

    // Cada proceso descubre cuántas filas recibirá
    int mis_filas;
    MPI_Scatter(filas_por_proceso, 1, MPI_INT, &mis_filas, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso prepara espacio para su submatriz
    int *mi_submatriz = (int*)malloc(mis_filas * N * sizeof(int));

    // Distribuir las filas usando MPI_Scatterv
    MPI_Scatterv(
            matriz_completa,        // Matriz completa a distribuir
            sendcounts,             // Número de ELEMENTOS para cada proceso
            desplazamientos,        // Desplazamientos en elementos
            MPI_INT,                // Tipo de dato
            mi_submatriz,           // Buffer local para recibir
            mis_filas * N,          // Cantidad de elementos a recibir
            MPI_INT,                // Tipo de dato
            0,                      // Proceso raíz
            MPI_COMM_WORLD          // Comunicador
    );

    // ===========================================
    // PARTE 4: CÁLCULO DE SUMAS LOCALES
    // ===========================================
    // Cada proceso calcula la suma de su submatriz
    int suma_local = sumar_submatriz(mi_submatriz, mis_filas, N);

    printf("Proceso %d: Recibí %d filas, suma local = %d\n",
           rank, mis_filas, suma_local);

    // Opcional: Mostrar la submatriz de cada proceso (solo para matrices pequeñas)
    if(M <= 8 && N <= 8) {
        imprimir_matriz(mi_submatriz, mis_filas, N, rank);
    }

    // ===========================================
    // PARTE 5: CÁLCULO DE LA SUMA TOTAL CON MPI_REDUCE
    // ===========================================
    int suma_total;

    // MPI_Reduce combina todas las sumas locales en una suma total en el proceso 0
    MPI_Reduce(
            &suma_local,            // Dato local a enviar (suma de cada proceso)
            &suma_total,            // Donde se almacena el resultado (solo en proceso 0)
            1,                      // Cantidad de elementos
            MPI_INT,                // Tipo de dato
            MPI_SUM,                // Operación: SUMA
            0,                      // Proceso que recibe el resultado
            MPI_COMM_WORLD          // Comunicador
    );

    // ===========================================
    // PARTE 6: VERIFICACIÓN Y RESULTADOS (solo proceso 0)
    // ===========================================
    if(rank == 0) {
        printf("\n=== RESULTADO FINAL ===\n");
        printf("Suma total de toda la matriz: %d\n", suma_total);

        // Verificación: calcular la suma directamente en el proceso 0
        int suma_verificacion = 0;
        for(int i = 0; i < M * N; i++) {
            suma_verificacion += matriz_completa[i];
        }

        printf("Verificación (cálculo directo en proceso 0): %d\n", suma_verificacion);

        if(suma_total == suma_verificacion) {
            printf("✓ Los resultados coinciden correctamente.\n");
        } else {
            printf("✗ ERROR: Los resultados NO coinciden.\n");
        }

        // Liberar memoria
        free(matriz_completa);
        free(filas_por_proceso);
        free(sendcounts);
        free(desplazamientos);
    }

    // ===========================================
    // PARTE 7: LIMPIEZA
    // ===========================================
    free(mi_submatriz);

    MPI_Finalize();
    return 0;
}