#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mpi.h>
#include <omp.h>
#include <time.h>

// Función para inicializar la matriz con valores aleatorios (solo proceso 0)
void inicializar_matriz(double *matriz, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            matriz[i * columnas + j] = (double)rand() / RAND_MAX * 100.0;
        }
    }
}

// Función para calcular suma de filas CON OpenMP
void calcular_suma_filas_openmp(double *matriz, double *sumas, int filas, int columnas) {
#pragma omp parallel for
    for (int i = 0; i < filas; i++) {
        double suma_local = 0.0;
        for (int j = 0; j < columnas; j++) {
            suma_local += matriz[i * columnas + j];
        }
        sumas[i] = suma_local;
    }
}

// Función para calcular suma de filas SIN OpenMP
void calcular_suma_filas_secuencial(double *matriz, double *sumas, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        double suma_local = 0.0;
        for (int j = 0; j < columnas; j++) {
            suma_local += matriz[i * columnas + j];
        }
        sumas[i] = suma_local;
    }
}

// Función para escribir resultados en archivo
void escribir_resultados_archivo(FILE *archivo, const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    vprintf(formato, args);
    if (archivo != NULL) {
        vfprintf(archivo, formato, args);
    }
    va_end(args);
}

int main(int argc, char *argv[]) {
    int rank, size;
    int filas_total, columnas;
    double *matriz = NULL;
    double *sumas_total = NULL;
    double t_inicio, t_fin, t_secuencial;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Verificar argumentos
    if (argc != 3) {
        if (rank == 0) {
            printf("Uso: %s <filas> <columnas>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    filas_total = atoi(argv[1]);
    columnas = atoi(argv[2]);

    if (filas_total <= 0 || columnas <= 0) {
        if (rank == 0) {
            printf("Error: Las dimensiones deben ser positivas\n");
        }
        MPI_Finalize();
        return 1;
    }

    // Inicializar semilla para números aleatorios (solo proceso 0)
    if (rank == 0) {
        srand(time(NULL));
    }

    // Archivo de resultados (solo proceso 0)
    FILE *archivo_resultados = NULL;
    time_t current_time;
    time(&current_time);

    if (rank == 0) {
        char nombre_archivo[100];
        snprintf(nombre_archivo, sizeof(nombre_archivo),
                 "resultados_mpi_openmp_%dx%d_%dprocesos.txt",
                 filas_total, columnas, size);
        archivo_resultados = fopen(nombre_archivo, "w");

        if (archivo_resultados == NULL) {
            printf("Error: No se pudo crear el archivo de resultados\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(archivo_resultados, "RESULTADOS PROGRAMA HÍBRIDO MPI + OpenMP\n");
        fprintf(archivo_resultados, "========================================\n");
        fprintf(archivo_resultados, "Fecha: %s", ctime(&current_time));
        fprintf(archivo_resultados, "Procesos MPI: %d\n", size);
        fprintf(archivo_resultados, "Matriz inicial: %d x %d\n\n", filas_total, columnas);
    }

    // Calcular distribución de filas
    int filas_por_proceso = filas_total / size;
    int filas_extra = filas_total % size;

    int filas_local = filas_por_proceso;
    if (rank < filas_extra) {
        filas_local++;
    }

    // Preparar arrays para Scatterv
    int *sendcounts = (int*)malloc(size * sizeof(int));
    int *displs = (int*)malloc(size * sizeof(int));

    // Arrays para Gatherv de las sumas
    int *recvcounts_sumas = (int*)malloc(size * sizeof(int));
    int *displs_sumas = (int*)malloc(size * sizeof(int));

    // Solo proceso 0 calcula los valores
    if (rank == 0) {
        int offset = 0;
        int offset_sumas = 0;
        for (int i = 0; i < size; i++) {
            int filas_proc = (i < filas_extra) ? filas_por_proceso + 1 : filas_por_proceso;
            sendcounts[i] = filas_proc * columnas;  // Número de elementos (doubles)
            displs[i] = offset;                     // Desplazamiento en elementos
            offset += sendcounts[i];

            recvcounts_sumas[i] = filas_proc;       // Número de sumas (una por fila)
            displs_sumas[i] = offset_sumas;         // Desplazamiento en sumas
            offset_sumas += recvcounts_sumas[i];
        }
    }

    // Broadcast de parámetros de distribución a TODOS los procesos
    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(recvcounts_sumas, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs_sumas, size, MPI_INT, 0, MPI_COMM_WORLD);

    // Proceso 0 inicializa la matriz completa
    if (rank == 0) {
        matriz = (double*)malloc(filas_total * columnas * sizeof(double));
        inicializar_matriz(matriz, filas_total, columnas);
        sumas_total = (double*)malloc(filas_total * sizeof(double));

        // Medir tiempo secuencial
        t_inicio = MPI_Wtime();
        calcular_suma_filas_secuencial(matriz, sumas_total, filas_total, columnas);
        t_fin = MPI_Wtime();
        t_secuencial = t_fin - t_inicio;

        escribir_resultados_archivo(archivo_resultados, "=== CONFIGURACIÓN INICIAL ===\n");
        escribir_resultados_archivo(archivo_resultados, "Matriz: %d x %d\n", filas_total, columnas);
        escribir_resultados_archivo(archivo_resultados, "Procesos MPI: %d\n", size);
        escribir_resultados_archivo(archivo_resultados, "Distribución de filas: ");
        for (int i = 0; i < size; i++) {
            escribir_resultados_archivo(archivo_resultados, "%d ", recvcounts_sumas[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n");
        escribir_resultados_archivo(archivo_resultados, "Tiempo secuencial: %.6f segundos\n\n", t_secuencial);

        // Verificar que las sumas sean correctas antes de continuar
        escribir_resultados_archivo(archivo_resultados, "Primeras 10 sumas de filas (verificación):\n");
        for (int i = 0; i < 10 && i < filas_total; i++) {
            escribir_resultados_archivo(archivo_resultados, "Fila %d: %.2f\n", i, sumas_total[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n");

        printf("DEBUG: Matriz %dx%d, Tiempo secuencial: %.6f\n", filas_total, columnas, t_secuencial);
        printf("DEBUG: Primera suma: %.2f\n", sumas_total[0]);
    }

    // Crear matriz local para cada proceso
    double *matriz_local = (double*)malloc(filas_local * columnas * sizeof(double));
    double *sumas_local = (double*)malloc(filas_local * sizeof(double));

    // Sincronizar antes de comenzar las pruebas
    MPI_Barrier(MPI_COMM_WORLD);

    // Solo proceso 0 realiza el análisis completo
    if (rank == 0) {
        escribir_resultados_archivo(archivo_resultados, "=== ANÁLISIS DE SPEEDUP ===\n");

        int config_hilos[] = {1, 2, 4, 8, 16};
        int num_configs = 5;

        escribir_resultados_archivo(archivo_resultados, "Configuraciones de hilos: ");
        for (int i = 0; i < num_configs; i++) {
            escribir_resultados_archivo(archivo_resultados, "%d ", config_hilos[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n\n");

        escribir_resultados_archivo(archivo_resultados, "Hilos_OpenMP,Tiempo_Secuencial,Tiempo_Paralelo,Speedup,Eficiencia\n");

        // Probar diferentes configuraciones de hilos
        for (int config_idx = 0; config_idx < num_configs; config_idx++) {
            int hilos_openmp = config_hilos[config_idx];

            // Establecer número de hilos para TODOS los procesos
            omp_set_num_threads(hilos_openmp);

            // Sincronizar antes de cada prueba
            MPI_Barrier(MPI_COMM_WORLD);
            double start_time = MPI_Wtime();

            // Re-distribuir datos para cada prueba
            MPI_Scatterv(
                    matriz, sendcounts, displs, MPI_DOUBLE,
                    matriz_local, sendcounts[rank], MPI_DOUBLE, // Usar sendcounts[rank] para este proceso
                    0, MPI_COMM_WORLD
            );

            // Calcular con OpenMP
            calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

            // Recolectar resultados - CORREGIDO: usar recvcounts_sumas y displs_sumas
            MPI_Gatherv(
                    sumas_local, recvcounts_sumas[rank], MPI_DOUBLE, // Cada proceso envía su número de filas
                    sumas_total, recvcounts_sumas, displs_sumas, MPI_DOUBLE,
                    0, MPI_COMM_WORLD
            );

            MPI_Barrier(MPI_COMM_WORLD);
            double end_time = MPI_Wtime();
            double parallel_time = end_time - start_time;

            if (t_secuencial > 0 && parallel_time > 0) {
                double speedup = t_secuencial / parallel_time;
                double eficiencia = (speedup / (size * hilos_openmp)) * 100;

                escribir_resultados_archivo(archivo_resultados,
                                            "%d,%.6f,%.6f,%.4f,%.2f\n",
                                            hilos_openmp, t_secuencial, parallel_time, speedup, eficiencia);

                printf("OpenMP %d hilos: Tiempo=%.6fs, Speedup=%.4f, Eficiencia=%.2f%%\n",
                       hilos_openmp, parallel_time, speedup, eficiencia);
            } else {
                escribir_resultados_archivo(archivo_resultados,
                                            "%d,%.6f,%.6f,ERROR,ERROR\n",
                                            hilos_openmp, t_secuencial, parallel_time);
                printf("ERROR: Tiempos inválidos para %d hilos\n", hilos_openmp);
            }
        }

        // Probar configuración automática
        int auto_threads = omp_get_max_threads();
        omp_set_num_threads(auto_threads);

        MPI_Barrier(MPI_COMM_WORLD);
        double start_time_auto = MPI_Wtime();

        MPI_Scatterv(
                matriz, sendcounts, displs, MPI_DOUBLE,
                matriz_local, sendcounts[rank], MPI_DOUBLE,
                0, MPI_COMM_WORLD
        );

        calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

        MPI_Gatherv(
                sumas_local, recvcounts_sumas[rank], MPI_DOUBLE,
                sumas_total, recvcounts_sumas, displs_sumas, MPI_DOUBLE,
                0, MPI_COMM_WORLD
        );

        MPI_Barrier(MPI_COMM_WORLD);
        double end_time_auto = MPI_Wtime();
        double auto_time = end_time_auto - start_time_auto;

        if (t_secuencial > 0 && auto_time > 0) {
            double speedup_auto = t_secuencial / auto_time;
            double eficiencia_auto = (speedup_auto / (size * auto_threads)) * 100;

            escribir_resultados_archivo(archivo_resultados,
                                        "Auto(%d),%.6f,%.6f,%.4f,%.2f\n",
                                        auto_threads, t_secuencial, auto_time, speedup_auto, eficiencia_auto);

            printf("OpenMP Auto(%d) hilos: Tiempo=%.6fs, Speedup=%.4f, Eficiencia=%.2f%%\n",
                   auto_threads, auto_time, speedup_auto, eficiencia_auto);
        }

        // Resumen
        escribir_resultados_archivo(archivo_resultados, "\n=== RESUMEN ===\n");
        escribir_resultados_archivo(archivo_resultados, "Procesos MPI: %d\n", size);
        escribir_resultados_archivo(archivo_resultados, "Hilos OpenMP disponibles: %d\n", omp_get_max_threads());
        escribir_resultados_archivo(archivo_resultados, "Matriz: %d x %d\n", filas_total, columnas);

        fclose(archivo_resultados);
        printf("\nResultados guardados en archivo\n");

    } else {
        // Procesos no-0 participan en las operaciones
        int config_hilos[] = {1, 2, 4, 8, 16};
        int num_configs = 5;

        for (int config_idx = 0; config_idx < num_configs; config_idx++) {
            int hilos_openmp = config_hilos[config_idx];
            omp_set_num_threads(hilos_openmp);

            MPI_Barrier(MPI_COMM_WORLD);

            // Los procesos no-0 reciben datos - CORREGIDO: usar sendcounts[rank]
            MPI_Scatterv(
                    NULL, sendcounts, displs, MPI_DOUBLE,
                    matriz_local, sendcounts[rank], MPI_DOUBLE,
                    0, MPI_COMM_WORLD
            );

            calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

            // Los procesos no-0 envían resultados - CORREGIDO: usar recvcounts_sumas[rank]
            MPI_Gatherv(
                    sumas_local, recvcounts_sumas[rank], MPI_DOUBLE,
                    NULL, NULL, NULL, MPI_DOUBLE,
                    0, MPI_COMM_WORLD
            );

            MPI_Barrier(MPI_COMM_WORLD);
        }

        // Configuración automática para procesos no-0
        int auto_threads = omp_get_max_threads();
        omp_set_num_threads(auto_threads);

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Scatterv(
                NULL, sendcounts, displs, MPI_DOUBLE,
                matriz_local, sendcounts[rank], MPI_DOUBLE,
                0, MPI_COMM_WORLD
        );

        calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

        MPI_Gatherv(
                sumas_local, recvcounts_sumas[rank], MPI_DOUBLE,
                NULL, NULL, NULL, MPI_DOUBLE,
                0, MPI_COMM_WORLD
        );

        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Sincronizar antes de liberar memoria
    MPI_Barrier(MPI_COMM_WORLD);

    // Liberar memoria local de CADA proceso
    free(matriz_local);
    free(sumas_local);

    // Liberar arrays de distribución
    free(sendcounts);
    free(displs);
    free(recvcounts_sumas);
    free(displs_sumas);

    // Solo proceso 0 libera la matriz global
    if (rank == 0) {
        if (matriz != NULL) {
            free(matriz);
        }
        if (sumas_total != NULL) {
            free(sumas_total);
        }
    }

    MPI_Finalize();
    return 0;
}