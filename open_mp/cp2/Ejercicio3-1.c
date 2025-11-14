#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <math.h>

#define MASTER 0
#define PI_REFERENCE 3.141592653589793238462643

// Función que define el integrando: f(x) = 4/(1 + x²)
double f(double x) {
    return 4.0 / (1.0 + x * x);
}

double calculate_pi_sequential(long num_steps) {
    double step = 1.0 / (double)num_steps;
    double sum = 0.0;
    
    for (long i = 0; i < num_steps; i++) {
        double x = (i + 0.5) * step;
        sum += f(x);
    }
    
    return sum * step;
}

double calculate_pi_hybrid(long num_steps, int mpi_rank, int mpi_size) {
    double step = 1.0 / (double)num_steps;
    double local_sum = 0.0;
    double global_sum = 0.0;
    
    // Calcular el rango local para cada proceso MPI
    long local_steps = num_steps / mpi_size;
    long start = mpi_rank * local_steps;
    long end = (mpi_rank == mpi_size - 1) ? num_steps : start + local_steps;
    
    // Paralelismo OpenMP dentro de cada proceso MPI
    #pragma omp parallel for reduction(+:local_sum)
    for (long i = start; i < end; i++) {
        double x = (i + 0.5) * step;
        local_sum += f(x);
    }
    
    // Reducción MPI para combinar resultados
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, MASTER, MPI_COMM_WORLD);
    
    return global_sum * step;
}

int main(int argc, char *argv[]) {
    int mpi_rank, mpi_size;
    long num_steps = 100000000; // Número de pasos por defecto
    
    // Leer número de pasos desde argumentos
    if (argc > 1) {
        num_steps = atol(argv[1]);
    }
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    
    // Solo el proceso maestro imprime información inicial
    if (mpi_rank == MASTER) {
        printf("=== CALCULO DE PI MEDIANTE INTEGRACION NUMERICA ===\n");
        printf("Valor de referencia: %.15f\n", PI_REFERENCE);
        printf("Numero de pasos: %ld\n", num_steps);
        printf("Numero de procesos MPI: %d\n", mpi_size);
        printf("Numero de hilos OpenMP por proceso: %d\n", omp_get_max_threads());
        printf("\n");
    }
    
    // Diferentes configuraciones de hilos OpenMP a probar
    int thread_configs[] = {1, 2, 4, 8};
    int num_configs = sizeof(thread_configs) / sizeof(thread_configs[0]);
    
    // Calcular π secuencialmente (solo en proceso maestro)
    double sequential_pi = 0.0;
    double sequential_time = 0.0;
    
    if (mpi_rank == MASTER) {
        printf("--- CALCULO SECUENCIAL ---\n");
        double start_time = MPI_Wtime();
        sequential_pi = calculate_pi_sequential(num_steps);
        double end_time = MPI_Wtime();
        sequential_time = end_time - start_time;
        
        double error = fabs(sequential_pi - PI_REFERENCE);
        printf("Pi calculado: %.15f\n", sequential_pi);
        printf("Error absoluto: %.15f\n", error);
        printf("Error relativo: %.15f%%\n", (error / PI_REFERENCE) * 100.0);
        printf("Tiempo secuencial: %.6f segundos\n", sequential_time);
        printf("\n");
    }
    
    // Broadcast del tiempo secuencial para cálculos consistentes
    MPI_Bcast(&sequential_time, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);
    
    // Archivo de resultados
    FILE *output_file = NULL;
    char filename[100];
    
    if (mpi_rank == MASTER) {
        snprintf(filename, sizeof(filename), "pi_calculation_results_%ld.txt", num_steps);
        output_file = fopen(filename, "w");
        
        if (output_file != NULL) {
            fprintf(output_file, "RESULTADOS CALCULO DE PI - METODO DE INTEGRACION NUMERICA\n");
            fprintf(output_file, "==========================================================\n");
            fprintf(output_file, "Valor de referencia: %.15f\n", PI_REFERENCE);
            fprintf(output_file, "Numero de pasos: %ld\n", num_steps);
            fprintf(output_file, "Procesos MPI: %d\n", mpi_size);
            fprintf(output_file, "Pi secuencial: %.15f\n", sequential_pi);
            fprintf(output_file, "Tiempo secuencial: %.6f segundos\n\n", sequential_time);
            
            fprintf(output_output_file, "CONFIGURACION | HILOS/PROCESO | TOTAL HILOS | PI CALCULADO | ERROR ABSOLUTO | ERROR RELATIVO%% | TIEMPO(s) | SPEEDUP | EFICIENCIA\n");
            fprintf(output_file, "----------------------------------------------------------------------------------------------------------------------------\n");
        }
        
        printf("=== CALCULO HIBRIDO (MPI + OpenMP) ===\n");
        printf("Configuración | Hilos/Proceso | Total Hilos | Pi Calculado   | Error Absoluto  | Error Relativo%% | Tiempo(s) | Speedup | Eficiencia\n");
        printf("----------------------------------------------------------------------------------------------------------------------------\n");
    }
    
    // Probar diferentes configuraciones de hilos
    for (int config = 0; config < num_configs; config++) {
        int num_threads = thread_configs[config];
        
        MPI_Barrier(MPI_COMM_WORLD);
        omp_set_num_threads(num_threads);
        
        double start_time_par = MPI_Wtime();
        double parallel_pi = calculate_pi_hybrid(num_steps, mpi_rank, mpi_size);
        double end_time_par = MPI_Wtime();
        double parallel_time = end_time_par - start_time_par;
        
        // Solo el proceso maestro procesa los resultados
        if (mpi_rank == MASTER) {
            double error_abs = fabs(parallel_pi - PI_REFERENCE);
            double error_rel = (error_abs / PI_REFERENCE) * 100.0;
            double speedup = sequential_time / parallel_time;
            int total_threads = mpi_size * num_threads;
            double efficiency = speedup / total_threads;
            
            printf("   %2d MPI × %2d |      %2d       |     %3d     | %.10f | %14.10f | %14.10f | %8.6f | %7.4f | %9.4f\n",
                mpi_size, num_threads, num_threads, total_threads,
                parallel_pi, error_abs, error_rel, parallel_time, speedup, efficiency);
            
            if (output_file != NULL) {
                fprintf(output_file, "   %2d MPI × %2d |      %2d       |     %3d     | %.10f | %14.10f | %14.10f | %8.6f | %7.4f | %9.4f\n",
                    mpi_size, num_threads, num_threads, total_threads,
                    parallel_pi, error_abs, error_rel, parallel_time, speedup, efficiency);
            }
        }
    }
    
    // Probar configuración automática
    MPI_Barrier(MPI_COMM_WORLD);
    omp_set_num_threads(omp_get_max_threads());
    
    double start_time_auto = MPI_Wtime();
    double auto_pi = calculate_pi_hybrid(num_steps, mpi_rank, mpi_size);
    double end_time_auto = MPI_Wtime();
    double auto_time = end_time_auto - start_time_auto;
    
    if (mpi_rank == MASTER) {
        double error_abs_auto = fabs(auto_pi - PI_REFERENCE);
        double error_rel_auto = (error_abs_auto / PI_REFERENCE) * 100.0;
        double speedup_auto = sequential_time / auto_time;
        int auto_threads = omp_get_max_threads();
        int total_auto_threads = mpi_size * auto_threads;
        double efficiency_auto = speedup_auto / total_auto_threads;
        
        printf("   %2d MPI × Auto |      %2d       |     %3d     | %.10f | %14.10f | %14.10f | %8.6f | %7.4f | %9.4f\n",
            mpi_size, auto_threads, total_auto_threads,
            auto_pi, error_abs_auto, error_rel_auto, auto_time, speedup_auto, efficiency_auto);
        
        if (output_file != NULL) {
            fprintf(output_file, "   %2d MPI × Auto |      %2d       |     %3d     | %.10f | %14.10f | %14.10f | %8.6f | %7.4f | %9.4f\n",
                mpi_size, auto_threads, total_auto_threads,
                auto_pi, error_abs_auto, error_rel_auto, auto_time, speedup_auto, efficiency_auto);
        }
        
        // Análisis de convergencia con diferentes números de pasos
        printf("\n=== ANALISIS DE CONVERGENCIA ===\n");
        printf("Pasos       | Pi Calculado   | Error Absoluto  | Error Relativo%%\n");
        printf("----------------------------------------------------------------\n");
        
        if (output_file != NULL) {
            fprintf(output_file, "\nANALISIS DE CONVERGENCIA\n");
            fprintf(output_file, "=======================\n");
            fprintf(output_file, "Pasos       | Pi Calculado   | Error Absoluto  | Error Relativo%%\n");
            fprintf(output_file, "----------------------------------------------------------------\n");
        }
        
        long step_sizes[] = {1000, 10000, 100000, 1000000, 10000000, 100000000};
        int num_step_sizes = sizeof(step_sizes) / sizeof(step_sizes[0]);
        
        for (int i = 0; i < num_step_sizes; i++) {
            long steps = step_sizes[i];
            double test_pi = calculate_pi_sequential(steps);
            double conv_error_abs = fabs(test_pi - PI_REFERENCE);
            double conv_error_rel = (conv_error_abs / PI_REFERENCE) * 100.0;
            
            printf("%10ld | %.10f | %14.10f | %14.10f\n",
                steps, test_pi, conv_error_abs, conv_error_rel);
            
            if (output_file != NULL) {
                fprintf(output_file, "%10ld | %.10f | %14.10f | %14.10f\n",
                    steps, test_pi, conv_error_abs, conv_error_rel);
            }
        }
        
        // Resumen y conclusiones
        printf("\n=== RESUMEN ===\n");
        printf("El método de integración numérica converge al valor real de π a medida que\n");
        printf("aumenta el número de pasos. La precisión mejora aproximadamente con O(1/n).\n");
        
        if (output_file != NULL) {
            fprintf(output_file, "\nRESUMEN\n");
            fprintf(output_file, "=======\n");
            fprintf(output_file, "El método de integración numérica para calcular π converge como O(1/n).\n");
            fprintf(output_file, "El error disminuye aproximadamente a la mitad cuando se duplica el número de pasos.\n");
            fprintf(output_file, "La implementación híbrida MPI+OpenMP permite acelerar significativamente el cálculo\n");
            fprintf(output_file, "manteniendo la precisión numérica del resultado.\n");
            
            fclose(output_file);
            printf("\nResultados guardados en: %s\n", filename);
        }
    }
    
    MPI_Finalize();
    return 0;
}