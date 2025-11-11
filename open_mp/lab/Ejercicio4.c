#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <limits.h>

#define ROWS 1000
#define COLS 1000

// Función para inicializar la matriz con valores aleatorios
void initialize_matrix(int matrix[ROWS][COLS], int bound) {
    srand(time(NULL));
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            matrix[i][j] = rand() % bound;
        }
    }
}

// Versión secuencial para encontrar min y max
void find_min_max_serial(int matrix[ROWS][COLS], int *min_val, int *max_val) {
    *min_val = INT_MAX;
    *max_val = INT_MIN;
    
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (matrix[i][j] < *min_val) {
                *min_val = matrix[i][j];
            }
            if (matrix[i][j] > *max_val) {
                *max_val = matrix[i][j];
            }
        }
    }
}

// Versión paralela usando reducción
void find_min_max_parallel_reduction(int matrix[ROWS][COLS], int *min_val, int *max_val) {
    int local_min = INT_MAX;
    int local_max = INT_MIN;
    
    #pragma omp parallel for reduction(min:local_min) reduction(max:local_max)
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (matrix[i][j] < local_min) {
                local_min = matrix[i][j];
            }
            if (matrix[i][j] > local_max) {
                local_max = matrix[i][j];
            }
        }
    }
    
    *min_val = local_min;
    *max_val = local_max;
}

int main() {
    int matrix[ROWS][COLS];
    int min_val, max_val;
    
    printf("EJERCICIO 4 - MENOR Y MAYOR ELEMENTO DE UNA MATRIZ\n");
    printf("==================================================\n");
    printf("Dimensiones: %d x %d (%d elementos)\n\n", ROWS, COLS, ROWS * COLS);
    
    // Inicializar matriz con valores entre 0 y 999
    initialize_matrix(matrix, 1000);
    
    double start_time, end_time, serial_time, parallel_time;
    
    // 1. Versión secuencial
    printf("\n=== VERSION SECUENCIAL ===\n");
    start_time = omp_get_wtime();
    find_min_max_serial(matrix, &min_val, &max_val);
    end_time = omp_get_wtime();
    serial_time = end_time - start_time;
    
    printf("Menor elemento: %d\n", min_val);
    printf("Mayor elemento: %d\n", max_val);
    printf("Tiempo secuencial: %.6f segundos\n", serial_time);
    
    // 2. Versión paralela con reducción
    printf("\n=== VERSION PARALELA ===\n");
    start_time = omp_get_wtime();
    find_min_max_parallel_reduction(matrix, &min_val, &max_val);
    end_time = omp_get_wtime();
    parallel_time = end_time - start_time;
    
    printf("Menor elemento: %d\n", min_val);
    printf("Mayor elemento: %d\n", max_val);
    printf("Tiempo paralelo: %.6f segundos\n", parallel_time);
    printf("Speedup: %.4f\n", serial_time / parallel_time);
    
    // 4. Comparación con diferentes números de hilos
    printf("\n=== COMPARACION CON DIFERENTES HILOS ===\n");
    printf("(Usando reducción)\n");
    printf("Hilos\tTiempo (s)\tSpeedup\t\tEficiencia\n");
    printf("---------------------------------------------\n");
    
    int num_threads_list[] = {2, 4, 8, 16};
    int num_threads_count = sizeof(num_threads_list) / sizeof(num_threads_list[0]);
    
    for (int i = 0; i < num_threads_count; i++) {
        int num_threads = num_threads_list[i];
        omp_set_num_threads(num_threads);
        
        start_time = omp_get_wtime();
        find_min_max_parallel_reduction(matrix, &min_val, &max_val);
        end_time = omp_get_wtime();
        parallel_time = end_time - start_time;
        
        double speedup = serial_time / parallel_time;
        double efficiency = speedup / num_threads;
        
        printf("%d\t%.6f\t%.4f\t\t%.4f\n", 
            num_threads, parallel_time, speedup, efficiency);
    }
    return 0;
}