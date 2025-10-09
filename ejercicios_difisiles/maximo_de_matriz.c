//
// Created by cjorg on 10/8/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>

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
 * Función para encontrar el elemento máximo en una submatriz
 */
int encontrar_maximo_local(int *submatriz, int filas, int columnas) {
    int maximo = INT_MIN;  // Inicializar con el valor mínimo posible

    for(int i = 0; i < filas; i++) {
        for(int j = 0; j < columnas; j++) {
            if(submatriz[i * columnas + j] > maximo) {
                maximo = submatriz[i * columnas + j];
            }
        }
    }
    return maximo;
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
        // Solicitar al usuario las dimensiones de la matriz
        printf("=== BÚSQUEDA DISTRIBUIDA DEL ELEMENTO MÁXIMO EN MATRIZ ===\n");

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

        // Llenar con valores aleatorios (0-999)
        srand(time(NULL));
        printf("Matriz completa (%dx%d):\n", M, N);
        for(int i = 0; i < M; i++) {
            for(int j = 0; j < N; j++) {
                matriz_completa[i * N + j] = rand() % 1000;
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

        int filas_base = M / size;
        int filas_extra = M % size;
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
    // PARTE 4: BÚSQUEDA DEL MÁXIMO LOCAL
    // ===========================================
    // Cada proceso encuentra el máximo en su submatriz
    int maximo_local = encontrar_maximo_local(mi_submatriz, mis_filas, N);

    printf("Proceso %d: Recibí %d filas, máximo local = %d\n",
           rank, mis_filas, maximo_local);

    // Opcional: Mostrar la submatriz de cada proceso (solo para matrices pequeñas)
    if(M <= 8 && N <= 8) {
        imprimir_matriz(mi_submatriz, mis_filas, N, rank);
    }

    // ===========================================
    // PARTE 5: RECOLECCIÓN DE MÁXIMOS Y CÁLCULO DEL MÁXIMO GLOBAL
    // ===========================================
    int *todos_los_maximos = NULL;

    if(rank == 0) {
        todos_los_maximos = (int*)malloc(size * sizeof(int));
    }

    // Recolectar todos los máximos locales en el proceso 0
    MPI_Gather(
            &maximo_local,          // Dato local a enviar
            1,                      // Cantidad de elementos
            MPI_INT,                // Tipo de dato
            todos_los_maximos,      // Buffer para recibir todos los máximos
            1,                      // Cantidad a recibir de cada proceso
            MPI_INT,                // Tipo de dato
            0,                      // Proceso destino
            MPI_COMM_WORLD          // Comunicador
    );

    // ===========================================
    // PARTE 6: ENCONTRAR EL MÁXIMO GLOBAL (solo proceso 0)
    // ===========================================
    if(rank == 0) {
        int maximo_global = todos_los_maximos[0];
        int proceso_maximo = 0;

        printf("\n=== RESULTADOS DE MÁXIMOS LOCALES ===\n");
        for(int i = 0; i < size; i++) {
            printf("Proceso %d: máximo local = %d\n", i, todos_los_maximos[i]);
            if(todos_los_maximos[i] > maximo_global) {
                maximo_global = todos_los_maximos[i];
                proceso_maximo = i;
            }
        }

        printf("\n=== RESULTADO FINAL ===\n");
        printf("El elemento máximo en toda la matriz es: %d\n", maximo_global);
        printf("Este elemento fue encontrado por el proceso %d\n", proceso_maximo);

        // Verificación: calcular el máximo directamente en el proceso 0 para validar
        int maximo_verificacion = INT_MIN;
        for(int i = 0; i < M * N; i++) {
            if(matriz_completa[i] > maximo_verificacion) {
                maximo_verificacion = matriz_completa[i];
            }
        }

        printf("Verificación (cálculo directo en proceso 0): %d\n", maximo_verificacion);

        if(maximo_global == maximo_verificacion) {
            printf("✓ Los resultados coinciden correctamente.\n");
        } else {
            printf("✗ ERROR: Los resultados NO coinciden.\n");
        }

        // Liberar memoria
        free(matriz_completa);
        free(filas_por_proceso);
        free(desplazamientos);
        free(todos_los_maximos);
    }

    // ===========================================
    // PARTE 7: LIMPIEZA
    // ===========================================
    free(mi_submatriz);

    MPI_Finalize();
    return 0;
}