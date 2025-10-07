#include <time.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

void inicializar_vectores(double *vector_a, double *vector_b, int longitud){
    for (int i = 0; i < longitud; i++){
        vector_a[i] = rand() % 10; // Valores aleatorios para el vector a
        printf("vector 1 posicion %d: %2.1f        ", i, vector_a[i]);
        vector_b[i] = rand() % 10; // Valores aleatorios para el vector b
        printf("vector 2 posicion %d: %2.1f ", i, vector_b[i]);
        printf("\n");
        fflush(stdout);
    }
}

int main(int argc, char *argv[]){
    int rank, np;
    int longitud_vectores = atoi(argv[1]); // Longitud de los vectores
    double *vector_a, *vector_b, *vector_local_a, *vector_local_b;
    double suma_local = 0.0, suma_global = 0.0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Obtener el rango del proceso actual
    MPI_Comm_size(MPI_COMM_WORLD, &np); // Obtener el número total de procesos

    // Inicializar vectores en el proceso 0
    if (rank == 0){
        vector_a = malloc(longitud_vectores * sizeof(double));
        vector_b = malloc(longitud_vectores * sizeof(double));
        srand(time(NULL));
        inicializar_vectores(vector_a, vector_b, longitud_vectores);
    }
    // Calcular cuántos elementos le corresponden a cada proceso
    int elementos_por_proceso = longitud_vectores / np;

    // Asignar espacio para los vectores locales
    vector_local_a = malloc(elementos_por_proceso * sizeof(double));
    vector_local_b = malloc(elementos_por_proceso * sizeof(double));

    // Scatter: distribuir los vectores a todos los procesos
    MPI_Scatter(vector_a, elementos_por_proceso, MPI_DOUBLE, vector_local_a, elementos_por_proceso, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatter(vector_b, elementos_por_proceso, MPI_DOUBLE, vector_local_b, elementos_por_proceso, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    double start_time = MPI_Wtime();//iniciando tiempo

// Calcular producto escalar localmente
#pragma omp parallel for reduction(+ : suma_local)
    for (int i = 0; i < elementos_por_proceso; i++){
        suma_local += vector_local_a[i] * vector_local_b[i]; // Sumar producto escalar local
    }
    double end_time = MPI_Wtime();//terminando tiempo

    // Recolectar resultados en el proceso 0
    MPI_Reduce(&suma_local, &suma_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Calcular y mostrar speedup solo en el proceso 0
    if (rank == 0){
        printf("Producto escalar: %6.1f \n", suma_global);
        free(vector_a);
        free(vector_b);
        printf("Tiempo total de ejecución paralela: %f segundos\n", end_time - start_time);

    }

    free(vector_local_a);
    free(vector_local_b);

    MPI_Finalize(); // Finalizar MPI
    return 0;
}