#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <math.h>

#define N 2000  // Dimensión de la matriz

// Función para calcular la norma de un vector
double vector_norm(double a[N]) {
    double norm = 0.0;
#pragma omp parallel for reduction(+:norm)
    for (int i = 0; i < N; i++) {
        norm += a[i] * a[i];
    }
    return sqrt(norm);
}

// Versión secuencial
void prodmv_serial(double a[N], double c[N], double B[N][N]) {
    int i, j;
    double sum;

    for (i = 0; i < N; i++) {
        sum = 0.0;
        for (j = 0; j < N; j++) {
            sum += B[i][j] * c[j];
        }
        a[i] = sum;
    }
}

// Versión paralela
void prodmv_parallel_outer(double a[N], double c[N], double B[N][N]) {
    int i, j;

#pragma omp parallel for private(j)
    for (i = 0; i < N; i++) {
        double sum = 0.0;
        for (j = 0; j < N; j++) {
            sum += B[i][j] * c[j];
        }
        a[i] = sum;
    }
}

// Función para inicializar datos
void initialize_data(double a[N], double c[N], double B[N][N]) {
    srand(time(NULL));

    for (int i = 0; i < N; i++) {
        c[i] = (double)rand() / RAND_MAX * 100.0;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = (double)rand() / RAND_MAX * 100.0;
        }
    }

    for (int i = 0; i < N; i++) {
        a[i] = 0.0;
    }
}

int main() {
    // Usar memoria contigua para la matriz B (compatible con double [N][N])
    double *a_serial = malloc(N * sizeof(double));
    double *a_parallel = malloc(N * sizeof(double));
    double *a_test = malloc(N * sizeof(double));
    double *c = malloc(N * sizeof(double));
    double (*B)[N] = malloc(N * sizeof(*B));  // Puntero a arrays de N elementos

    // Verificar asignación de memoria
    if (!a_serial || !a_parallel || !a_test || !c || !B) {
        printf("Error: No se pudo asignar memoria\n");
        return 1;
    }

    printf("EJERCICIO 3 - PRODUCTO MATRIZ-VECTOR\n");
    printf("====================================\n");
    printf("Dimension: N = %d\n\n", N);

    // Inicializar datos
    initialize_data(a_serial, c, B);

    double start_time, end_time, serial_time, parallel_time;

    printf("=== PARTE A: Programa Paralelo OpenMP ===\n");

    // Ejecutar versión secuencial
    printf("Ejecutando version secuencial...\n");
    start_time = omp_get_wtime();
    prodmv_serial(a_serial, c, B);
    end_time = omp_get_wtime();
    serial_time = end_time - start_time;
    printf("Tiempo secuencial: %.6f segundos\n", serial_time);
    printf("Norma del resultado: %.6f\n\n", vector_norm(a_serial));

    // Ejecutar versión paralela
    printf("Ejecutando version paralela...\n");
    start_time = omp_get_wtime();
    prodmv_parallel_outer(a_parallel, c, B);
    end_time = omp_get_wtime();
    parallel_time = end_time - start_time;
    printf("Tiempo paralelo: %.6f segundos\n", parallel_time);
    printf("Norma del resultado: %.6f\n\n", vector_norm(a_parallel));

    // Cálculo de Speedup y Eficiencia
    printf("=== PARTE B: Speedup y Eficiencia ===\n");
    printf("Hilos\tTiempo (s)\tSpeedup\t\tEficiencia\n");
    printf("---------------------------------------------\n");

    int num_threads_list[] = {1, 2, 4, 8, 16};
    int num_threads_count = sizeof(num_threads_list) / sizeof(num_threads_list[0]);

    for (int i = 0; i < num_threads_count; i++) {
        int num_threads = num_threads_list[i];
        omp_set_num_threads(num_threads);

        for (int k = 0; k < N; k++) {
            a_test[k] = 0.0;
        }

        start_time = omp_get_wtime();
        prodmv_parallel_outer(a_test, c, B);
        end_time = omp_get_wtime();
        parallel_time = end_time - start_time;

        double speedup = serial_time / parallel_time;
        double efficiency = speedup / num_threads;

        printf("%d\t%.6f\t%.4f\t\t%.4f\n",
               num_threads, parallel_time, speedup, efficiency);
    }

    // Liberar memoria
    free(a_serial);
    free(a_parallel);
    free(a_test);
    free(c);
    free(B);  // Solo necesitamos un free para B

    return 0;
}