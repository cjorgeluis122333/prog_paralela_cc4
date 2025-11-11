#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <math.h>

#define N 10000  // Rango máximo para buscar primos

// Función secuencial
int prime_number_serial(int n) {
    int i;
    int j;
    int prime;
    int total = 0;

    for (i = 2; i <= n; i++) {
        prime = 1;

        for (j = 2; j < i; j++) {
            if (i % j == 0) {
                prime = 0;
                break;
            }
        }
        total = total + prime;
    }
    return total;
}

// Versión paralela
int prime_number_parallel(int n) {
    int total = 0;
    
    #pragma omp parallel for reduction(+:total)
    for (int i = 2; i <= n; i++) {
        int prime = 1;
        
        for (int j = 2; j < i; j++) {
            if (i % j == 0) {
                prime = 0;
                break;
            }
        }
        total += prime;
    }
    return total;
}

int main() {
    printf("EJERCICIO 5 - CONTADOR DE NUMEROS PRIMOS EN PARALELO\n");
    printf("====================================================\n");
    printf("Rango: [1, %d]\n\n", N);
    
    double start_time, end_time, serial_time, parallel_time;
    int prime_count;
    
    // 1. Versión secuencial
    printf("=== VERSION SECUENCIAL ===\n");
    start_time = omp_get_wtime();
    prime_count = prime_number_serial(N);
    end_time = omp_get_wtime();
    serial_time = end_time - start_time;
    
    printf("Cantidad de primos: %d\n", prime_count);
    printf("Tiempo secuencial: %.6f segundos\n\n", serial_time);
    
    // 2. Versión paralela
    printf("=== VERSION PARALELA ===\n");
    start_time = omp_get_wtime();
    prime_count = prime_number_parallel(N);
    end_time = omp_get_wtime();
    parallel_time = end_time - start_time;
    
    printf("Cantidad de primos: %d\n", prime_count);
    printf("Tiempo paralelo: %.6f segundos\n", parallel_time);
    printf("Speedup vs serial básico: %.4f\n\n", serial_time / parallel_time);
    
    // 3. Comparación con diferentes números de hilos
    printf("=== COMPARACION CON DIFERENTES HILOS ===\n");
    printf("(Usando versión paralela)\n");
    printf("Hilos\tTiempo (s)\tSpeedup\t\tEficiencia\n");
    printf("---------------------------------------------\n");
    
    int num_threads_list[] = {2, 4, 8, 16};
    int num_threads_count = sizeof(num_threads_list) / sizeof(num_threads_list[0]);
    
    for (int i = 0; i < num_threads_count; i++) {
        int num_threads = num_threads_list[i];
        omp_set_num_threads(num_threads);
        
        start_time = omp_get_wtime();
        prime_count = prime_number_parallel(N);
        end_time = omp_get_wtime();
        parallel_time = end_time - start_time;
        
        double speedup = serial_time / parallel_time;
        double efficiency = speedup / num_threads;
        
        printf("%d\t%.6f\t%.4f\t\t%.4f\n", 
            num_threads, parallel_time, speedup, efficiency);
    }
    return 0;
}