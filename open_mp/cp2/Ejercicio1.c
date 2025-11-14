#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mpi.h>
#include <omp.h>
#include <time.h>

// Función para inicializar la matriz con valores aleatorios (solo proceso 0)
void inicializar_matriz(double **matriz, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            matriz[i][j] = (double)rand() / RAND_MAX * 100.0;
        }
    }
}

// Función para calcular suma de filas CON OpenMP
void calcular_suma_filas_openmp(double **matriz, double *sumas, int filas, int columnas) {
#pragma omp parallel for
    for (int i = 0; i < filas; i++) {
        double suma_local = 0.0;
        for (int j = 0; j < columnas; j++) {
            suma_local += matriz[i][j];
        }
        sumas[i] = suma_local;
    }
}

// Función para calcular suma de filas SIN OpenMP
void calcular_suma_filas_secuencial(double **matriz, double *sumas, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        double suma_local = 0.0;
        for (int j = 0; j < columnas; j++) {
            suma_local += matriz[i][j];
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
    double **matriz = NULL;
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
    int *sendcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));

        int offset = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = (i < filas_extra) ? filas_por_proceso + 1 : filas_por_proceso;
            displs[i] = offset;
            offset += sendcounts[i];
        }
    } else {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));
    }

    // Broadcast de parámetros de distribución
    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

    // Proceso 0 inicializa la matriz completa
    if (rank == 0) {
        matriz = (double**)malloc(filas_total * sizeof(double*));
        for (int i = 0; i < filas_total; i++) {
            matriz[i] = (double*)malloc(columnas * sizeof(double));
        }
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
            fprintf(archivo_resultados, "%d ", sendcounts[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n");
        escribir_resultados_archivo(archivo_resultados, "Tiempo secuencial: %.6f segundos\n\n", t_secuencial);

        escribir_resultados_archivo(archivo_resultados, "Primeras 10 sumas de filas:\n");
        for (int i = 0; i < 10 && i < filas_total; i++) {
            escribir_resultados_archivo(archivo_resultados, "Fila %d: %.2f\n", i, sumas_total[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n");
    }

    // Crear matriz local para cada proceso
    double **matriz_local = (double**)malloc(filas_local * sizeof(double*));
    for (int i = 0; i < filas_local; i++) {
        matriz_local[i] = (double*)malloc(columnas * sizeof(double));
    }
    double *sumas_local = (double*)malloc(filas_local * sizeof(double));

    // Crear tipo de dato MPI para una fila
    MPI_Datatype fila_type;
    MPI_Type_contiguous(columnas, MPI_DOUBLE, &fila_type);
    MPI_Type_commit(&fila_type);

    // Distribuir datos
    MPI_Scatterv(
            (rank == 0) ? matriz[0] : NULL,
            sendcounts, displs, fila_type,
            matriz_local[0], filas_local, fila_type,
            0, MPI_COMM_WORLD
    );

    // Solo proceso 0 realiza el análisis completo
    if (rank == 0) {
        escribir_resultados_archivo(archivo_resultados, "=== ANÁLISIS DE SPEEDUP ===\n");

        int config_hilos[] = {1, 2, 4, 8, 16};
        int num_configs = 5;

        escribir_resultados_archivo(archivo_resultados, "Configuraciones de hilos: ");
        for (int i = 0; i < num_configs; i++) {
            fprintf(archivo_resultados, "%d ", config_hilos[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n\n");

        fprintf(archivo_resultados, "Hilos_OpenMP,Tiempo_Secuencial,Tiempo_Paralelo,Speedup,Eficiencia\n");

        // Probar diferentes configuraciones de hilos
        for (int config_idx = 0; config_idx < num_configs; config_idx++) {
            int hilos_openmp = config_hilos[config_idx];
            omp_set_num_threads(hilos_openmp);

            MPI_Barrier(MPI_COMM_WORLD);
            double start_time = MPI_Wtime();

            // Re-distribuir datos (ya están distribuidos, pero para consistencia)
            MPI_Scatterv(
                    matriz[0], sendcounts, displs, fila_type,
                    matriz_local[0], filas_local, fila_type,
                    0, MPI_COMM_WORLD
            );

            // Calcular con OpenMP
            calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

            // Recolectar resultados
            MPI_Gatherv(
                    sumas_local, filas_local, MPI_DOUBLE,
                    sumas_total, sendcounts, displs, MPI_DOUBLE,
                    0, MPI_COMM_WORLD
            );

            MPI_Barrier(MPI_COMM_WORLD);
            double end_time = MPI_Wtime();
            double parallel_time = end_time - start_time;

            double speedup = t_secuencial / parallel_time;
            double eficiencia = (speedup / (size * hilos_openmp)) * 100;

            escribir_resultados_archivo(archivo_resultados,
                                        "%d,%.6f,%.6f,%.4f,%.2f\n",
                                        hilos_openmp, t_secuencial, parallel_time, speedup, eficiencia);

            printf("OpenMP %d hilos: Tiempo=%.6fs, Speedup=%.4f, Eficiencia=%.2f%%\n",
                   hilos_openmp, parallel_time, speedup, eficiencia);
        }

        // Probar configuración automática
        MPI_Barrier(MPI_COMM_WORLD);
        omp_set_num_threads(omp_get_max_threads());

        double start_time_auto = MPI_Wtime();

        MPI_Scatterv(
                matriz[0], sendcounts, displs, fila_type,
                matriz_local[0], filas_local, fila_type,
                0, MPI_COMM_WORLD
        );

        calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

        MPI_Gatherv(
                sumas_local, filas_local, MPI_DOUBLE,
                sumas_total, sendcounts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD
        );

        MPI_Barrier(MPI_COMM_WORLD);
        double end_time_auto = MPI_Wtime();
        double auto_time = end_time_auto - start_time_auto;

        double speedup_auto = t_secuencial / auto_time;
        int auto_threads = omp_get_max_threads();
        double eficiencia_auto = (speedup_auto / (size * auto_threads)) * 100;

        escribir_resultados_archivo(archivo_resultados,
                                    "Auto(%d),%.6f,%.6f,%.4f,%.2f\n",
                                    auto_threads, t_secuencial, auto_time, speedup_auto, eficiencia_auto);

        printf("OpenMP Auto(%d) hilos: Tiempo=%.6fs, Speedup=%.4f, Eficiencia=%.2f%%\n",
               auto_threads, auto_time, speedup_auto, eficiencia_auto);

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
            double start_time = MPI_Wtime();

            MPI_Scatterv(
                    NULL, sendcounts, displs, fila_type,
                    matriz_local[0], filas_local, fila_type,
                    0, MPI_COMM_WORLD
            );

            calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

            MPI_Gatherv(
                    sumas_local, filas_local, MPI_DOUBLE,
                    NULL, sendcounts, displs, MPI_DOUBLE,
                    0, MPI_COMM_WORLD
            );

            MPI_Barrier(MPI_COMM_WORLD);
            double end_time = MPI_Wtime();
        }

        // Configuración automática
        MPI_Barrier(MPI_COMM_WORLD);
        omp_set_num_threads(omp_get_max_threads());

        double start_time_auto = MPI_Wtime();

        MPI_Scatterv(
                NULL, sendcounts, displs, fila_type,
                matriz_local[0], filas_local, fila_type,
                0, MPI_COMM_WORLD
        );

        calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);

        MPI_Gatherv(
                sumas_local, filas_local, MPI_DOUBLE,
                NULL, sendcounts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD
        );

        MPI_Barrier(MPI_COMM_WORLD);
        double end_time_auto = MPI_Wtime();
    }

    // Liberar memoria
    for (int i = 0; i < filas_local; i++) {
        free(matriz_local[i]);
    }
    free(matriz_local);
    free(sumas_local);
    free(sendcounts);
    free(displs);

    if (rank == 0) {
        for (int i = 0; i < filas_total; i++) {
            free(matriz[i]);
        }
        free(matriz);
        free(sumas_total);
    }

    MPI_Type_free(&fila_type);
    MPI_Finalize();

    return 0;
}