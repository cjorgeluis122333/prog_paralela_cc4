#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void inicializar_matriz(double *matriz, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            matriz[i * columnas + j] = rand() % 100; // Valores aleatorios
            printf("%6.1f ", matriz[i * columnas + j]);
        }
        printf("\n");
    }
    printf("\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    int rank, np;
    if (argc != 3) {
        printf("Uso: %s <filas> <columnas>\n", argv[0]);
        return -1;
    }

    int filas = atoi(argv[1]);    // Número de filas
    int columnas = atoi(argv[2]); // Número de columnas

    // Inicializar MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Obtener el rango del proceso actual
    MPI_Comm_size(MPI_COMM_WORLD, &np);   // Obtener el número total de procesos

    double *matriz = NULL;
    double *sumaporfila = NULL, *suma_local;

    if (rank == 0) {
        matriz = (double *)malloc(columnas * filas * sizeof(double));
        srand(time(NULL)); // Inicializar la semilla solo en el proceso 0
        inicializar_matriz(matriz, filas, columnas);
        sumaporfila = malloc(filas * sizeof(double)); // Almacenar las sumas de cada fila
    }

    // Calcular cuántas filas le corresponde a cada proceso
    int filas_base = filas / np;
    int filas_resto = filas % np;

    // Distribuir las filas a cada proceso
    int filas_asignadas = filas_base + (rank < filas_resto ? 1 : 0); // Distribuir el resto de las filas

    // Asignar memoria para la suma local
    suma_local = malloc(filas_asignadas * sizeof(double));

    int *filas_por_proceso = (int *)malloc(np * sizeof(int));
    int *desplazamiento = (int *)malloc(np * sizeof(int));

    for (int i = 0; i < np; i++) {
        if (i < filas_resto) {
            filas_por_proceso[i] = (filas_base + 1)*columnas; // Si el índice es menor que el resto, recibe una fila adicional
        } else {
            filas_por_proceso[i] = filas_base*columnas; // De lo contrario, recibe solo el número base de filas
        }

        desplazamiento[i] = i * filas_base + (i < filas_resto ? i : filas_resto);
    }

 // Crear un buffer para recibir las filas dispersadas
    double *buffer_recibido =(double *) malloc(filas_por_proceso[rank] * sizeof(double));

 //Scatterv para distribuir a cada proceso la cantidad de elementos 
    MPI_Scatterv(matriz, filas_por_proceso, desplazamiento, MPI_DOUBLE,
                  buffer_recibido,filas_por_proceso[rank], MPI_DOUBLE,
                  0, MPI_COMM_WORLD);

    double start_time = MPI_Wtime();//iniciando tiempo
    // Cada proceso calcula la suma de sus filas recibidas
#pragma omp parallel for
    for (int i = 0; i < filas_asignadas; i++) {
        suma_local[i] = 0;
        for (int j = 0; j < columnas; j++) {
            suma_local[i] += buffer_recibido[i * columnas + j]; // Sumar los elementos de la fila local
        }
    }

    double end_time = MPI_Wtime();//terminando tiempo
    // Recolectar resultados en el proceso 0
    MPI_Gatherv(suma_local, filas_asignadas, MPI_DOUBLE,
                sumaporfila, filas_por_proceso,
                desplazamiento, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    // Calcular y mostrar resultados solo en el proceso 0
    if (rank == 0) {
        printf("Suma total por fila:\n");
        for (int i = 0; i < filas; i++) {
            printf("Suma de la fila %d: %6.1f\n", i, sumaporfila[i]);
            fflush(stdout);
        }
        printf("Tiempo total de ejecución paralela: %f segundos\n", end_time - start_time);
        free(sumaporfila);
        free(matriz);
    }

    free(buffer_recibido);
    free(suma_local);
    free(filas_por_proceso);
    free(desplazamiento);

    MPI_Finalize(); 
    return 0;
}
