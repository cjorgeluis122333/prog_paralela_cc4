//
// Created by cjorg on 10/8/2025.
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
    // PARTE 1: LECTURA DEL TAMAÑO DE LA MATRIZ
    // ===========================================
    int N = 0;  // Tamaño de la matriz cuadrada (N x N)

    if(rank == 0) {
        // Solicitar al usuario el tamaño de la matriz
        printf("=== SUMA DISTRIBUIDA DE MATRIZ CUADRADA ===\n");

        if(argc > 1) {
            // Si se proporciona como argumento
            N = atoi(argv[1]);
            printf("Tamaño de matriz leído de argumento: %dx%d\n", N, N);
        } else {
            // Solicitar por entrada estándar
            printf("Ingrese el tamaño de la matriz cuadrada (N): ");
            scanf("%d", &N);
            printf("Matriz de tamaño: %dx%d\n", N, N);
        }

        // Validar que N sea positivo
        if(N <= 0) {
            printf("Error: El tamaño de la matriz debe ser positivo.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Número de procesos: %d\n\n", size);
    }

    // Distribuir el tamaño N a todos los procesos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Validar que la matriz pueda ser distribuida
    if(N == 0) {
        MPI_Finalize();
        return 0;
    }

    // ===========================================
    // PARTE 2: CREACIÓN Y LLENADO DE LA MATRIZ
    // ===========================================
    int *matriz_completa = NULL;

    if(rank == 0) {
        // Crear matriz N x N
        matriz_completa = (int*)malloc(N * N * sizeof(int));

        // Llenar con valores aleatorios (0-99)
        srand(time(NULL));
        printf("Matriz completa (%dx%d):\n", N, N);
        for(int i = 0; i < N; i++) {
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
    int *desplazamientos = NULL;

    if(rank == 0) {
        filas_por_proceso = (int*)malloc(size * sizeof(int));
        desplazamientos = (int*)malloc(size * sizeof(int));

        int filas_base = N / size;
        int filas_extra = N % size;
        int desplazamiento_actual = 0;

        printf("Distribución de filas:\n");
        for(int i = 0; i < size; i++) {
            filas_por_proceso[i] = filas_base;
            if(i < filas_extra) {
                filas_por_proceso[i]++;  // Procesos iniciales reciben fila extra
            }
            desplazamientos[i] = desplazamiento_actual * N;  // En elementos, no en filas
            desplazamiento_actual += filas_por_proceso[i];

            printf("  Proceso %d: %d filas (desplazamiento: %d)\n",
                   i, filas_por_proceso[i], desplazamientos[i]);
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
            filas_por_proceso,      // Número de filas para cada proceso
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

    // Opcional: Mostrar la submatriz de cada proceso
    if(N <= 10) {  // Solo si la matriz es pequeña
        imprimir_matriz(mi_submatriz, mis_filas, N, rank);
    }

    // ===========================================
    // PARTE 5: RECOLECCIÓN DE SUMAS Y CÁLCULO DEL MÁXIMO
    // ===========================================
    int *todas_las_sumas = NULL;

    if(rank == 0) {
        todas_las_sumas = (int*)malloc(size * sizeof(int));
    }

    // Recolectar todas las sumas en el proceso 0
    MPI_Gather(
            &suma_local,            // Dato local a enviar
            1,                      // Cantidad de elementos
            MPI_INT,                // Tipo de dato
            todas_las_sumas,        // Buffer para recibir todas las sumas
            1,                      // Cantidad a recibir de cada proceso
            MPI_INT,                // Tipo de dato
            0,                      // Proceso destino
            MPI_COMM_WORLD          // Comunicador
    );

    // ===========================================
    // PARTE 6: ENCONTRAR LA MAYOR SUMA (solo proceso 0)
    // ===========================================
    if(rank == 0) {
        int mayor_suma = todas_las_sumas[0];
        int proceso_mayor = 0;

        printf("\n=== RESULTADOS DE SUMAS ===\n");
        for(int i = 0; i < size; i++) {
            printf("Proceso %d: suma = %d\n", i, todas_las_sumas[i]);
            if(todas_las_sumas[i] > mayor_suma) {
                mayor_suma = todas_las_sumas[i];
                proceso_mayor = i;
            }
        }

        printf("\n=== RESULTADO FINAL ===\n");
        printf("La mayor suma es: %d (calculada por el proceso %d)\n",
               mayor_suma, proceso_mayor);

        // Calcular la suma total para verificación
        int suma_total = 0;
        for(int i = 0; i < size; i++) {
            suma_total += todas_las_sumas[i];
        }
        printf("Suma total de toda la matriz: %d\n", suma_total);

        // Liberar memoria
        free(matriz_completa);
        free(filas_por_proceso);
        free(desplazamientos);
        free(todas_las_sumas);
    }

    // ===========================================
    // PARTE 7: LIMPIEZA
    // ===========================================
    free(mi_submatriz);

    MPI_Finalize();
    return 0;
}