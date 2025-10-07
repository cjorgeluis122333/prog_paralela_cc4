#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

void initialize_vector(int* vec, int size) {
    for (int j = 0; j < size; j++) {
        vec[j] = rand() % 100 - 50; 
    }
}

int main(int argc, char** argv) {
    int vector_size, rank, num_processes;
    int *random_vector = NULL, total_sum = 0, partial_sum = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, 
                  &rank);
    MPI_Comm_size(MPI_COMM_WORLD, 
                  &num_processes);
    if (rank == 0) 
    {
        printf("Ingrese la longitud del vector: ");
        scanf("%d", &vector_size);
        random_vector = (int*)malloc(sizeof(int) * vector_size);
        srand(time(NULL));
        initialize_vector(random_vector, vector_size);
    }
    MPI_Bcast(&vector_size, 
              1, 
              MPI_INT, 
              0, 
              MPI_COMM_WORLD);
    int elements_per_process = vector_size / num_processes;
    int *local_vector = (int*)malloc(sizeof(int) * elements_per_process);
    MPI_Scatter(random_vector, 
                elements_per_process, 
                MPI_INT, 
                local_vector,
                elements_per_process, 
                MPI_INT, 
                0, 
                MPI_COMM_WORLD);
    for (int k = 0; k < elements_per_process; k++) 
    {
        partial_sum += local_vector[k];
    }
    printf("Proceso %d - Suma parcial: %d\n", rank, partial_sum);
    MPI_Reduce(&partial_sum, 
               &total_sum, 
               1, 
               MPI_INT, 
               MPI_SUM, 
               0, 
               MPI_COMM_WORLD);
    if (rank == 0) 
    {
        for (int l = 0; l < vector_size; l++) 
        {
            random_vector[l] *= total_sum;
        }
        printf("Suma total: %d\n", total_sum);
        printf("Vector multiplicado por la suma total: ");
        for (int m = 0; m < vector_size; m++) {
            printf("%d ", random_vector[m]);
        }
        printf("\n");
        
        free(random_vector); 
    }
    free(local_vector); 
    MPI_Finalize();
    return 0;
}
