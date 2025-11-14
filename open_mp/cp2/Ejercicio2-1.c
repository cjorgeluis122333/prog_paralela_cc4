#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // Incluido para usar fabs()
#include <omp.h>
#include <mpi.h>
#include <time.h>

#define MASTER 0
/**
 *
# Para compilar
mpicc -fopenmp -o ejer2 Ejercicio2-1.c -lm

# Ejecutar con 4 procesos MPI
mpirun -np 4 ./ejer2 1000000
 * @param u
 * @param v
 * @param n
 * @return
 */
double dot_product_sequential(double *u, double *v, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += u[i] * v[i];
    }
    return sum;
}

double dot_product_openmp(double *u_local, double *v_local, int local_n) {
    double local_sum = 0.0;

    // SOLO aquí se usa OpenMP - paralelismo a nivel de hilos
#pragma omp parallel for reduction(+:local_sum)
    for (int i = 0; i < local_n; i++) {
        local_sum += u_local[i] * v_local[i];
    }

    return local_sum;
}

int main(int argc, char *argv[]) {
    int mpi_rank, mpi_size;
    int n = 1000000; // Tamaño por defecto de los vectores

    // Leer tamaño de los vectores desde argumentos
    if (argc > 1) {
        n = atoi(argv[1]);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    // SOLO el proceso 0 inicializa los vectores completos usando MPI
    double *u_full = NULL;
    double *v_full = NULL;

    if (mpi_rank == MASTER) {
        u_full = (double*)malloc(n * sizeof(double));
        v_full = (double*)malloc(n * sizeof(double));

        // Inicializar vectores completos en el proceso maestro
        for (int i = 0; i < n; i++) {
            u_full[i] = 1.0; // Valores de ejemplo
            v_full[i] = 2.0; // Valores de ejemplo
        }
    }

    // Calcular distribución usando MPI
    int local_n = n / mpi_size;
    int remaining = n % mpi_size;

    // Preparar arrays para Scatterv - solo en maestro
    int *sendcounts = NULL;
    int *displs = NULL;

    if (mpi_rank == MASTER) {
        sendcounts = (int*)malloc(mpi_size * sizeof(int));
        displs = (int*)malloc(mpi_size * sizeof(int));

        int offset = 0;
        for (int i = 0; i < mpi_size; i++) {
            sendcounts[i] = local_n;
            if (i < remaining) {
                sendcounts[i]++; // Los primeros 'remaining' procesos obtienen un elemento extra
            }
            displs[i] = offset;
            offset += sendcounts[i];
        }
    }

    // Cada proceso descubre su tamaño local usando MPI_Scatter
    int local_size;
    MPI_Scatter(sendcounts, 1, MPI_INT, &local_size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

    // Cada proceso reserva memoria solo para su parte local
    double *u_local = (double*)malloc(local_size * sizeof(double));
    double *v_local = (double*)malloc(local_size * sizeof(double));

    // Distribuir partes de los vectores a cada proceso usando MPI_Scatterv
    MPI_Scatterv(u_full, sendcounts, displs, MPI_DOUBLE,
                 u_local, local_size, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);
    MPI_Scatterv(v_full, sendcounts, displs, MPI_DOUBLE,
                 v_local, local_size, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

    // Medir tiempo secuencial (solo en proceso maestro) - versión secuencial pura
    double start_time_seq = 0.0, end_time_seq = 0.0;
    double sequential_result = 0.0;
    double sequential_time = 0.0;

    // Variables para el archivo de salida
    FILE *output_file = NULL;
    char filename[100];
    time_t current_time;
    struct tm *time_info;

    if (mpi_rank == MASTER) {
        // Crear nombre de archivo con timestamp
        time(&current_time);
        time_info = localtime(&current_time);
        snprintf(filename, sizeof(filename),
                 "dot_product_results_%d_%04d%02d%02d_%02d%02d%02d.txt",
                 n,
                 time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
                 time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

        // Abrir archivo para escritura
        output_file = fopen(filename, "w");
        if (output_file == NULL) {
            printf("Error: No se pudo crear el archivo de resultados.\n");
        } else {
            printf("Resultados guardados en: %s\n", filename);
        }

        // Escribir cabecera en archivo
        if (output_file != NULL) {
            fprintf(output_file, "RESULTADOS PRODUCTO ESCALAR HÍBRIDO (MPI + OpenMP)\n");
            fprintf(output_file, "==================================================\n");
            fprintf(output_file, "Fecha: %04d-%02d-%02d %02d:%02d:%02d\n",
                    time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
                    time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
            fprintf(output_file, "Tamaño del vector: %d\n", n);
            fprintf(output_file, "Procesos MPI: %d\n", mpi_size);
            fprintf(output_file, "Hilos OpenMP máximos disponibles: %d\n\n", omp_get_max_threads());
        }

        printf("Calculando producto escalar para vectores de tamaño: %d\n", n);
        printf("Numero de procesos MPI: %d\n", mpi_size);

        start_time_seq = MPI_Wtime();
        sequential_result = dot_product_sequential(u_full, v_full, n);
        end_time_seq = MPI_Wtime();
        sequential_time = end_time_seq - start_time_seq;

        printf("\n--- CONFIGURACION SECUENCIAL ---\n");
        printf("Resultado secuencial: %.6f\n", sequential_result);
        printf("Tiempo secuencial: %.6f segundos\n", sequential_time);

        // Escribir información secuencial en archivo
        if (output_file != NULL) {
            fprintf(output_file, "CONFIGURACION SECUENCIAL\n");
            fprintf(output_file, "------------------------\n");
            fprintf(output_file, "Resultado: %.6f\n", sequential_result);
            fprintf(output_file, "Tiempo: %.6f segundos\n\n", sequential_time);
        }
    }

    // Broadcast del tiempo secuencial a todos los procesos para cálculos consistentes
    MPI_Bcast(&sequential_time, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

    // Diferentes configuraciones de hilos OpenMP a probar
    int thread_configs[] = {1, 2, 4, 8, 16};
    int num_configs = sizeof(thread_configs) / sizeof(thread_configs[0]);

    // Arrays para almacenar resultados (solo en maestro)
    double *parallel_times = NULL;
    double *speedups = NULL;
    double *efficiencies = NULL;

    if (mpi_rank == MASTER) {
        parallel_times = (double*)malloc(num_configs * sizeof(double));
        speedups = (double*)malloc(num_configs * sizeof(double));
        efficiencies = (double*)malloc(num_configs * sizeof(double));

        printf("\n=== ANALISIS DE SPEEDUP CON DIFERENTES HILOS OPENMP ===\n");
        printf("Tamaño del vector: %d\n", n);
        printf("Procesos MPI: %d\n\n", mpi_size);
        printf("Configuración | Hilos/Proceso | Total Hilos | Tiempo (s) | Speedup  | Eficiencia\n");
        printf("-------------------------------------------------------------------------------\n");

        // Escribir cabecera de tabla en archivo
        if (output_file != NULL) {
            fprintf(output_file, "ANALISIS DE RENDIMIENTO CON DIFERENTES CONFIGURACIONES\n");
            fprintf(output_file, "======================================================\n");
            fprintf(output_file, "Configuración | Hilos/Proceso | Total Hilos | Tiempo (s) | Speedup  | Eficiencia\n");
            fprintf(output_file, "--------------------------------------------------------------------------------\n");
        }
    }

    // Probar diferentes configuraciones de hilos
    for (int config = 0; config < num_configs; config++) {
        int num_threads = thread_configs[config];

        // Sincronizar antes de cada configuración
        MPI_Barrier(MPI_COMM_WORLD);

        // Establecer número de hilos para OpenMP
        omp_set_num_threads(num_threads);

        // Medir tiempo para esta configuración
        double start_time_par = MPI_Wtime();

        // Cada proceso calcula su producto escalar local usando OpenMP
        double local_dot = dot_product_openmp(u_local, v_local, local_size);

        // Recolectar resultados usando MPI_Reduce
        double parallel_result;
        MPI_Reduce(&local_dot, &parallel_result, 1, MPI_DOUBLE, MPI_SUM, MASTER, MPI_COMM_WORLD);

        double end_time_par = MPI_Wtime();
        double parallel_time = end_time_par - start_time_par;

        // Calcular métricas de rendimiento (solo en proceso maestro)
        if (mpi_rank == MASTER) {
            double speedup = sequential_time / parallel_time;
            int total_threads = mpi_size * num_threads;
            double efficiency = speedup / total_threads;

            // Almacenar resultados
            parallel_times[config] = parallel_time;
            speedups[config] = speedup;
            efficiencies[config] = efficiency;

            printf("   %2d MPI × %2d |      %2d       |     %3d     | %8.6f | %7.4f |   %6.4f\n",
                   mpi_size, num_threads, num_threads, total_threads,
                   parallel_time, speedup, efficiency);

            // Escribir en archivo
            if (output_file != NULL) {
                fprintf(output_file, "   %2d MPI × %2d |      %2d       |     %3d     | %8.6f | %7.4f |   %6.4f\n",
                        mpi_size, num_threads, num_threads, total_threads,
                        parallel_time, speedup, efficiency);
            }

            // Verificar que el resultado sea correcto
            if (config == 0) {
                double error = fabs(sequential_result - parallel_result);
                if (error > 1e-10) {
                    printf("¡ADVERTENCIA! Diferencia en resultados: %.12f\n", error);
                    if (output_file != NULL) {
                        fprintf(output_file, "¡ADVERTENCIA! Diferencia en resultados: %.12f\n", error);
                    }
                }
            }
        }
    }

    // Probar también configuración con número automático de hilos
    MPI_Barrier(MPI_COMM_WORLD);
    omp_set_num_threads(omp_get_max_threads());

    double start_time_auto = MPI_Wtime();
    double local_dot_auto = dot_product_openmp(u_local, v_local, local_size);
    double parallel_result_auto;
    MPI_Reduce(&local_dot_auto, &parallel_result_auto, 1, MPI_DOUBLE, MPI_SUM, MASTER, MPI_COMM_WORLD);
    double end_time_auto = MPI_Wtime();
    double parallel_time_auto = end_time_auto - start_time_auto;

    if (mpi_rank == MASTER) {
        double speedup_auto = sequential_time / parallel_time_auto;
        int auto_threads = omp_get_max_threads();
        int total_auto_threads = mpi_size * auto_threads;
        double efficiency_auto = speedup_auto / total_auto_threads;

        printf("   %2d MPI × Auto |      %2d       |     %3d     | %8.6f | %7.4f |   %6.4f\n",
               mpi_size, auto_threads, total_auto_threads,
               parallel_time_auto, speedup_auto, efficiency_auto);

        // Escribir configuración automática en archivo
        if (output_file != NULL) {
            fprintf(output_file, "   %2d MPI × Auto |      %2d       |     %3d     | %8.6f | %7.4f |   %6.4f\n",
                    mpi_size, auto_threads, total_auto_threads,
                    parallel_time_auto, speedup_auto, efficiency_auto);
        }

        // Encontrar mejor configuración
        int best_config = 0;
        double best_efficiency = efficiencies[0];
        for (int i = 1; i < num_configs; i++) {
            if (efficiencies[i] > best_efficiency) {
                best_efficiency = efficiencies[i];
                best_config = i;
            }
        }

        printf("\n--- RESUMEN ---\n");
        printf("Mejor configuración encontrada para %d procesos MPI:\n", mpi_size);
        printf("- Hilos OpenMP: %d\n", thread_configs[best_config]);
        printf("- Speedup: %.4f\n", speedups[best_config]);
        printf("- Eficiencia: %.4f\n", best_efficiency);
        printf("- Configuración: %d MPI × %d OpenMP\n", mpi_size, thread_configs[best_config]);

        // Escribir resumen en archivo
        if (output_file != NULL) {
            fprintf(output_file, "\nRESUMEN Y RECOMENDACIONES\n");
            fprintf(output_file, "========================\n");
            fprintf(output_file, "Mejor configuración para %d procesos MPI:\n", mpi_size);
            fprintf(output_file, "- Hilos OpenMP recomendados: %d\n", thread_configs[best_config]);
            fprintf(output_file, "- Speedup máximo: %.4f\n", speedups[best_config]);
            fprintf(output_file, "- Eficiencia máxima: %.4f\n", best_efficiency);
            fprintf(output_file, "- Configuración óptima: %d MPI × %d OpenMP\n", mpi_size, thread_configs[best_config]);
            fprintf(output_file, "- Tiempo secuencial: %.6f segundos\n", sequential_time);
            fprintf(output_file, "- Tiempo paralelo óptimo: %.6f segundos\n", parallel_times[best_config]);

            // Cerrar archivo
            fclose(output_file);
            printf("Resultados guardados exitosamente en: %s\n", filename);
        }

        // Liberar memoria de los vectores completos y arrays auxiliares (solo en maestro)
        free(u_full);
        free(v_full);
        free(sendcounts);
        free(displs);
        free(parallel_times);
        free(speedups);
        free(efficiencies);
    }

    // Liberar memoria local en todos los procesos
    free(u_local);
    free(v_local);

    MPI_Finalize();

    return 0;
}