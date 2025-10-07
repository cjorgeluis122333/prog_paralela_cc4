#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

int main(int argc, char** argv) {
    int N, i;
    int *V1 = NULL, sum = 0, sumaparcial = 0;
    int proceso, np;

    // Inicializar MPI y obtener proceso actual y cantidad de procesos
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &proceso);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    // Solo el proceso 0 pide la longitud del vector, asigna memoria e inicializa semilla para numeros aleatorios y el vector
    if (proceso == 0) {
        printf("\nLongitud del vector: ");
        scanf("%d", &N); 
        V1 = (int*)malloc(sizeof(int) * N); 
        srand(time(NULL)); 

        for (i = 0; i < N; i++) {
            V1[i] = rand() % 100 - 50; // Valores generados aleatoriamente en el mismo intervalo que en la orden del ejercicio
        }
        
        // Imprimir el vector original
        printf("\nVector original: ");
        for (i = 0; i < N; i++) {
            printf("%d ", V1[i]);
        }
        printf("\n");
    }

    // Broadcast de la longitud del vector a todos los procesos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Distribuir el vector entre todos los procesos
    int elementos = N / np;
    int *V2 = (int*)malloc(sizeof(int) * elementos);
    
    // Scatter el vector a todos los procesos
    MPI_Scatter(V1, elementos, MPI_INT, V2, elementos, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso calcula su suma parcial
    for (i = 0; i < elementos; i++) {
        sumaparcial += V2[i];
    }

    // Imprimir la suma parcial en cada proceso
    printf("Suma parcial del proceso %d: %d\n", proceso,sumaparcial);

    // Reducir las sumas locales al proceso 0
    MPI_Reduce(&sumaparcial, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // El proceso 0 multiplica el vector por la suma total y lo imprime
    if (proceso == 0) {
        for (i = 0; i < N; i++) {
            V1[i] *= sum;
        }
        printf("\nSuma total: %d\n", sum);

        printf("\nVector multiplicado por la suma total: ");
        for (i = 0; i < N; i++) {
            printf("%d ", V1[i]);
        }
        printf("\n");

        free(V1); // Liberar memoria de v1 solo en proceso 0 donde fue inicializado
    }

    free(V2); // Liberar memoria en todos los procesos

    // Finalizar el entorno MPI
    MPI_Finalize();
    
    return 0;
}
