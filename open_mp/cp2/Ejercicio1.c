#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <time.h>

// Función para inicializar la matriz con valores aleatorios (solo proceso 0)
void inicializar_matriz(double **matriz, int filas, int columnas) {
    srand(time(NULL)); // Semilla única para proceso 0
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            matriz[i][j] = (double)rand() / RAND_MAX * 100.0; // Valores entre 0 y 100
        }
    }
}

// Función para calcular suma de filas CON OpenMP (solo esta parte usa OpenMP)
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

// Función para calcular suma de filas SIN OpenMP (para comparación)
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
    vprintf(formato, args);  // Escribir en consola
    vfprintf(archivo, formato, args);  // Escribir en archivo
    va_end(args);
}

int main(int argc, char *argv[]) {
    int rank, size;
    int filas_total, columnas;
    double **matriz = NULL;
    double *sumas_total = NULL;
    double t_inicio, t_fin, t_secuencial, t_paralelo;
    
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
    
    // Abrir archivo de resultados (solo proceso 0)
    FILE *archivo_resultados = NULL;
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
        
        // Escribir cabecera del archivo
        fprintf(archivo_resultados, "RESULTADOS PROGRAMA HÍBRIDO MPI + OpenMP\n");
        fprintf(archivo_resultados, "========================================\n");
        fprintf(archivo_resultados, "Fecha: %s", ctime(&(time_t){time(NULL)}));
        fprintf(archivo_resultados, "Procesos MPI: %d\n", size);
        fprintf(archivo_resultados, "Matriz inicial: %d x %d\n\n", filas_total, columnas);
    }
    
    // Calcular distribución de filas (solución para caso A - no múltiplos)
    int filas_por_proceso = filas_total / size;
    int filas_extra = filas_total % size;
    
    int filas_local = filas_por_proceso;
    if (rank < filas_extra) {
        filas_local++;
    }
    
    // Preparar arrays de conteo y desplazamiento para Scatterv
    int *sendcounts = (int*)malloc(size * sizeof(int));
    int *displs = (int*)malloc(size * sizeof(int));
    
    int current_displ = 0;
    for (int i = 0; i < size; i++) {
        int count = (i < filas_extra) ? filas_por_proceso + 1 : filas_por_proceso;
        sendcounts[i] = count;
        displs[i] = current_displ;
        current_displ += count;
    }
    
    // Broadcast de los parámetros de distribución a todos los procesos
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
            printf("%d ", sendcounts[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n");
        escribir_resultados_archivo(archivo_resultados, "Tiempo secuencial de referencia: %.6f segundos\n\n", t_secuencial);
        
        // Escribir algunas sumas para verificación
        escribir_resultados_archivo(archivo_resultados, "Primeras 10 sumas de filas (verificación):\n");
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
    
    // Crear tipo de dato MPI para una fila completa
    MPI_Datatype fila_type;
    MPI_Type_contiguous(columnas, MPI_DOUBLE, &fila_type);
    MPI_Type_commit(&fila_type);
    
    // Distribuir datos de la matriz
    MPI_Scatterv(
        (rank == 0) ? matriz[0] : NULL,
        sendcounts, displs, fila_type,
        matriz_local[0], filas_local, fila_type,
        0, MPI_COMM_WORLD
    );
    
    if (rank == 0) {
        escribir_resultados_archivo(archivo_resultados, "=== ANÁLISIS DE SPEEDUP CON DIFERENTES HILOS OPENMP ===\n");
        
        // Diferentes configuraciones de hilos
        int config_hilos[] = {1, 2, 4, 8, 16};
        int num_configs = 5;
        
        // Diferentes dimensiones de matriz
        int dimensiones_filas[] = {1000, 5000, 10000};
        int dimensiones_columnas[] = {500, 2500, 5000};
        int num_dim = 3;
        
        escribir_resultados_archivo(archivo_resultados, "Configuraciones de hilos OpenMP a probar: ");
        for (int i = 0; i < num_configs; i++) {
            fprintf(archivo_resultados, "%d ", config_hilos[i]);
            printf("%d ", config_hilos[i]);
        }
        escribir_resultados_archivo(archivo_resultados, "\n\n");
        
        // Crear tabla de resultados para análisis posterior
        fprintf(archivo_resultados, "TABLA DE RESULTADOS - FORMATO CSV\n");
        fprintf(archivo_resultados, "Dimension,Hilos_OpenMP,Tiempo_Secuencial,Tiempo_Paralelo,Speedup,Eficiencia\n");
        
        for (int dim_idx = 0; dim_idx < num_dim; dim_idx++) {
            int dim_filas = dimensiones_filas[dim_idx];
            int dim_columnas = dimensiones_columnas[dim_idx];
            
            escribir_resultados_archivo(archivo_resultados, "=== DIMENSIÓN %d x %d ===\n", dim_filas, dim_columnas);
            
            if (rank == 0) {
                // Reasignar memoria para nueva dimensión
                for (int i = 0; i < filas_total; i++) {
                    free(matriz[i]);
                }
                free(matriz);
                
                matriz = (double**)malloc(dim_filas * sizeof(double*));
                for (int i = 0; i < dim_filas; i++) {
                    matriz[i] = (double*)malloc(dim_columnas * sizeof(double));
                }
                
                // Tiempo secuencial para esta dimensión
                inicializar_matriz(matriz, dim_filas, dim_columnas);
                t_inicio = MPI_Wtime();
                calcular_suma_filas_secuencial(matriz, sumas_total, dim_filas, dim_columnas);
                t_fin = MPI_Wtime();
                double ts_local = t_fin - t_inicio;
                
                escribir_resultados_archivo(archivo_resultados, "Tiempo secuencial: %.6f s\n", ts_local);
            }
            
            // Probar diferentes configuraciones de hilos
            for (int config_idx = 0; config_idx < num_configs; config_idx++) {
                int hilos_openmp = config_hilos[config_idx];
                
                // Establecer número de hilos para todos los procesos
                omp_set_num_threads(hilos_openmp);
                
                // Calcular nueva distribución para esta dimensión
                int fpp_new = dim_filas / size;
                int fe_new = dim_filas % size;
                int fl_new = (rank < fe_new) ? fpp_new + 1 : fpp_new;
                
                // Recalcular sendcounts y displs
                current_displ = 0;
                for (int i = 0; i < size; i++) {
                    int count = (i < fe_new) ? fpp_new + 1 : fpp_new;
                    sendcounts[i] = count;
                    displs[i] = current_displ;
                    current_displ += count;
                }
                
                // Reasignar memoria local si es necesario
                if (fl_new != filas_local) {
                    for (int i = 0; i < filas_local; i++) {
                        free(matriz_local[i]);
                    }
                    free(matriz_local);
                    
                    matriz_local = (double**)malloc(fl_new * sizeof(double*));
                    for (int i = 0; i < fl_new; i++) {
                        matriz_local[i] = (double*)malloc(dim_columnas * sizeof(double));
                    }
                    
                    free(sumas_local);
                    sumas_local = (double*)malloc(fl_new * sizeof(double));
                    
                    filas_local = fl_new;
                }
                
                // Sincronizar antes de medir tiempo
                MPI_Barrier(MPI_COMM_WORLD);
                t_inicio = MPI_Wtime();
                
                // Distribuir datos
                MPI_Scatterv(
                    (rank == 0) ? matriz[0] : NULL,
                    sendcounts, displs, fila_type,
                    matriz_local[0], fl_new, fila_type,
                    0, MPI_COMM_WORLD
                );
                
                // CALCULAR CON (con diferente número de hilos)
                calcular_suma_filas_openmp(matriz_local, sumas_local, fl_new, dim_columnas);
                
                // Recolectar resultados
                MPI_Gatherv(
                    sumas_local, fl_new, MPI_DOUBLE,
                    (rank == 0) ? sumas_total : NULL,
                    sendcounts, displs, MPI_DOUBLE,
                    0, MPI_COMM_WORLD
                );
                
                MPI_Barrier(MPI_COMM_WORLD);
                t_fin = MPI_Wtime();
                
                if (rank == 0) {
                    double tp_local = t_fin - t_inicio;
                    
                    // Calcular métricas de performance
                    double speedup = ts_local / tp_local;
                    double eficiencia = (speedup / (size * hilos_openmp)) * 100;
                    
                    escribir_resultados_archivo(archivo_resultados, 
                        "  OpenMP %2d hilos: Tp=%.6fs, Speedup=%.4f, Eficiencia=%.2f%%\n", 
                        hilos_openmp, tp_local, speedup, eficiencia);
                    
                    // Escribir en formato CSV para análisis posterior
                    fprintf(archivo_resultados, "%dx%d,%d,%.6f,%.6f,%.4f,%.2f\n",
                            dim_filas, dim_columnas, hilos_openmp, ts_local, tp_local, speedup, eficiencia);
                }
            }
            escribir_resultados_archivo(archivo_resultados, "\n");
        }
        
        // ANÁLISIS CON DIFERENTES HILOS - DIMENSIÓN FIJA
        escribir_resultados_archivo(archivo_resultados, "\n=== ANÁLISIS DE SCALING CON DIMENSIÓN FIJA ===\n");
        escribir_resultados_archivo(archivo_resultados, "Usando matriz %d x %d\n\n", filas_total, columnas);
        
        // Re-inicializar la matriz original
        inicializar_matriz(matriz, filas_total, columnas);
        
        // Cabecera CSV para dimensión fija
        fprintf(archivo_resultados, "TABLA DIMENSIÓN FIJA - FORMATO CSV\n");
        fprintf(archivo_resultados, "Hilos_OpenMP,Tiempo_Secuencial,Tiempo_Paralelo,Speedup,Eficiencia\n");
        
        for (int config_idx = 0; config_idx < num_configs; config_idx++) {
            int hilos_openmp = config_hilos[config_idx];
            
            // Establecer número de hilos
            omp_set_num_threads(hilos_openmp);
            
            MPI_Barrier(MPI_COMM_WORLD);
            t_inicio = MPI_Wtime();
            
            // Redistribuir datos originales
            MPI_Scatterv(
                (rank == 0) ? matriz[0] : NULL,
                sendcounts, displs, fila_type,
                matriz_local[0], filas_local, fila_type,
                0, MPI_COMM_WORLD
            );
            
            // Calcular
            calcular_suma_filas_openmp(matriz_local, sumas_local, filas_local, columnas);
            
            // Recolectar resultados
            MPI_Gatherv(
                sumas_local, filas_local, MPI_DOUBLE,
                (rank == 0) ? sumas_total : NULL,
                sendcounts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD
            );
            
            MPI_Barrier(MPI_COMM_WORLD);
            t_fin = MPI_Wtime();
            
            if (rank == 0) {
                double tp_local = t_fin - t_inicio;
                double speedup = t_secuencial / tp_local;
                double eficiencia = (speedup / (size * hilos_openmp)) * 100;
                
                escribir_resultados_archivo(archivo_resultados,
                    "OpenMP %2d hilos: Tp=%.6fs, Speedup=%.4f, Eficiencia=%.2f%%\n", 
                    hilos_openmp, tp_local, speedup, eficiencia);
                
                // Escribir en CSV
                fprintf(archivo_resultados, "%d,%.6f,%.6f,%.4f,%.2f\n",
                        hilos_openmp, t_secuencial, tp_local, speedup, eficiencia);
            }
        }
        
        // RESUMEN FINAL
        escribir_resultados_archivo(archivo_resultados, "\n=== RESUMEN FINAL ===\n");
        escribir_resultados_archivo(archivo_resultados, "Procesos MPI: %d\n", size);
        escribir_resultados_archivo(archivo_resultados, "Mejor configuración depende del balance entre:\n");
        escribir_resultados_archivo(archivo_resultados, "- Número de procesos MPI\n");
        escribir_resultados_archivo(archivo_resultados, "- Número de hilos OpenMP por proceso\n");
        escribir_resultados_archivo(archivo_resultados, "- Tamaño de la matriz\n");
        escribir_resultados_archivo(archivo_resultados, "- Overhead de comunicación MPI\n");
        escribir_resultados_archivo(archivo_resultados, "- Overhead de creación de hilos OpenMP\n");
        
        // Cerrar archivo
        fclose(archivo_resultados);
        printf("\nResultados guardados en el archivo: resultados_mpi_openmp_%dx%d_%dprocesos.txt\n", 
               filas_total, columnas, size);
        
    } else {
        // Procesos no-0 participan en todas las pruebas
        int config_hilos[] = {1, 2, 4, 8, 16};
        int num_configs = 5;
        int dimensiones_filas[] = {1000, 5000, 10000};
        int num_dim = 3;
        
        for (int dim_idx = 0; dim_idx < num_dim; dim_idx++) {
            int dim_filas = dimensiones_filas[dim_idx];
            
            for (int config_idx = 0; config_idx < num_configs; config_idx++) {
                int hilos_openmp = config_hilos[config_idx];
                omp_set_num_threads(hilos_openmp);
                
                int fpp_new = dim_filas / size;
                int fe_new = dim_filas % size;
                int fl_new = (rank < fe_new) ? fpp_new + 1 : fpp_new;
                
                if (fl_new != filas_local) {
                    for (int i = 0; i < filas_local; i++) {
                        free(matriz_local[i]);
                    }
                    free(matriz_local);
                    
                    matriz_local = (double**)malloc(fl_new * sizeof(double*));
                    for (int i = 0; i < fl_new; i++) {
                        matriz_local[i] = (double*)malloc(columnas * sizeof(double));
                    }
                    
                    free(sumas_local);
                    sumas_local = (double*)malloc(fl_new * sizeof(double));
                    
                    filas_local = fl_new;
                }
                
                MPI_Barrier(MPI_COMM_WORLD);
                t_inicio = MPI_Wtime();
                
                MPI_Scatterv(
                    NULL, sendcounts, displs, fila_type,
                    matriz_local[0], fl_new, fila_type,
                    0, MPI_COMM_WORLD
                );
                
                calcular_suma_filas_openmp(matriz_local, sumas_local, fl_new, columnas);
                
                MPI_Gatherv(
                    sumas_local, fl_new, MPI_DOUBLE,
                    NULL, sendcounts, displs, MPI_DOUBLE,
                    0, MPI_COMM_WORLD
                );
                
                MPI_Barrier(MPI_COMM_WORLD);
                t_fin = MPI_Wtime();
            }
        }
        
        // Participar en análisis de dimension fija
        for (int config_idx = 0; config_idx < num_configs; config_idx++) {
            int hilos_openmp = config_hilos[config_idx];
            omp_set_num_threads(hilos_openmp);
            
            MPI_Barrier(MPI_COMM_WORLD);
            t_inicio = MPI_Wtime();
            
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
            t_fin = MPI_Wtime();
        }
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