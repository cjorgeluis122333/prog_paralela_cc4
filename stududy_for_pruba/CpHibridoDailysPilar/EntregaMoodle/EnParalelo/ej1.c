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

    // Verificar que el número de filas es múltiplo del número de procesos
    if (filas % np != 0) {
        if (rank == 0) {
            printf("El número de filas debe ser múltiplo del número de procesos.\n");
        }
        MPI_Finalize();
        return -1;
    }

    double *matriz = NULL;
    double *sumaporfila = NULL, *suma_local;

    if (rank == 0) {
        matriz = (double *)malloc(columnas * filas * sizeof(double));
        srand(time(NULL));
        inicializar_matriz(matriz, filas, columnas);
        sumaporfila = malloc(filas * sizeof(double)); // Almacenar las sumas de cada fila
    }

    // Calcular cuántas filas le corresponde a cada proceso
    int filas_base = filas / np;
    
    // Asignar memoria para la suma local
    suma_local = malloc(filas_base * sizeof(double));

    // Crear un buffer para recibir las filas dispersadas
    double *buffer_recibido = malloc(filas_base * columnas * sizeof(double));

    // Scatter para distribuir las filas de la matriz desde el proceso raíz a todos los procesos
    MPI_Scatter(matriz, filas_base * columnas, MPI_DOUBLE, buffer_recibido, filas_base * columnas, MPI_DOUBLE, 0, MPI_COMM_WORLD);

   double start_time = MPI_Wtime();//iniciando tiempo

    // Cada proceso calcula la suma de sus filas recibidas
#pragma omp parallel for
    for (int i = 0; i < filas_base; i++) {
        suma_local[i] = 0;
        for (int j = 0; j < columnas; j++) {
            suma_local[i] += buffer_recibido[i * columnas + j]; // Sumar los elementos de la fila local
        }
    }
   double end_time = MPI_Wtime();//terminando tiempo
    // Recolectar resultados en el proceso 0
    MPI_Gather(suma_local, filas_base, MPI_DOUBLE, sumaporfila, filas_base, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Calcular y mostrar resultados solo en el proceso 0
    if (rank == 0) {
        printf("Suma de las filas:\n");
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

    MPI_Finalize(); 
    return 0;
}
