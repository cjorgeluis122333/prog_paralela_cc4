#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Función para inicializar el arreglo con valores aleatorios
void randInit(int array[], const int n, const int bound) {
    srand(time(NULL));
    for (int i = 0; i < n; i++)
        array[i] = rand() % bound;
}

// Versión serie del cálculo de la media
double mean_serial(int array[], const int n) {
    double sum = 0;
    for (int i = 0; i < n; i++)
        sum += array[i];
    return (double)sum / n;
}

// Versión paralela del cálculo de la media
double mean_parallel(int array[], const int n) {
    double sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < n; i++)
        sum += array[i];
    return (double)sum / n;
}

// Versión paralela con diferentes distribuciones y chunks
double mean_parallel_chunk(int array[], const int n, int chunk_size) {
    double sum = 0;
    #pragma omp parallel for reduction(+:sum) schedule(static, chunk_size)
    for (int i = 0; i < n; i++)
        sum += array[i];
    return (double)sum / n;
}

// Versión con distribución dinámica
double mean_parallel_dynamic(int array[], const int n, int chunk_size) {
    double sum = 0;
    #pragma omp parallel for reduction(+:sum) schedule(dynamic, chunk_size)
    for (int i = 0; i < n; i++)
        sum += array[i];
    return (double)sum / n;
}

int main() {
    const int n = 10000000;  // Tamaño grande del arreglo
    const int bound = 100;   // Límite para valores aleatorios
    int *array = (int*)malloc(n * sizeof(int));
    
    if (array == NULL) {
        printf("Error al asignar memoria\n");
        return 1;
    }
    
    // Inicializar arreglo
    randInit(array, n, bound);
    
    printf("Calculando media para n = %d\n\n", n);
    
    // A) Cálculo del speedup para diferentes números de hilos
    printf("=== PARTE A: Speedup para diferentes hilos ===\n");
    
    double start_time, end_time, serial_time, parallel_time;
    
    // Tiempo de ejecución serial
    start_time = omp_get_wtime();
    double serial_mean = mean_serial(array, n);
    end_time = omp_get_wtime();
    serial_time = end_time - start_time;
    
    printf("Tiempo serial: %.6f segundos\n", serial_time);
    printf("Media calculada: %.6f\n\n", serial_mean);
    
    int num_threads_list[] = {2, 4, 8, 16};
    int num_threads_count = sizeof(num_threads_list) / sizeof(num_threads_list[0]);
    
    for (int i = 0; i < num_threads_count; i++) {
        int num_threads = num_threads_list[i];
        omp_set_num_threads(num_threads);
        
        start_time = omp_get_wtime();
        double parallel_mean = mean_parallel(array, n);
        end_time = omp_get_wtime();
        parallel_time = end_time - start_time;
        
        double speedup = serial_time / parallel_time;
        
        printf("Hilos: %d\n", num_threads);
        printf("Tiempo paralelo: %.6f segundos\n", parallel_time);
        printf("Speedup: %.4f\n", speedup);
        printf("Media calculada: %.6f\n\n", parallel_mean);
    }
    
    // B) Diferentes distribuciones y tamaños de fragmento
    printf("\n=== PARTE B: Diferentes distribuciones y chunks ===\n");
    
    int chunk_sizes[] = {1, 100, 1000, 10000};
    int num_chunks = sizeof(chunk_sizes) / sizeof(chunk_sizes[0]);
    
    omp_set_num_threads(4);  // Usar 4 hilos
    
    for (int i = 0; i < num_chunks; i++) {
        int chunk_size = chunk_sizes[i];
        
        // Distribución estática
        start_time = omp_get_wtime();
        double mean_static = mean_parallel_chunk(array, n, chunk_size);
        end_time = omp_get_wtime();
        printf("Static, chunk=%d: %.6f segundos\n", chunk_size, end_time - start_time);
        
        // Distribución dinámica
        start_time = omp_get_wtime();
        double mean_dynamic = mean_parallel_dynamic(array, n, chunk_size);
        end_time = omp_get_wtime();
        printf("Dynamic, chunk=%d: %.6f segundos\n", chunk_size, end_time - start_time);
        
    }
    
    free(array);
    return 0;
}