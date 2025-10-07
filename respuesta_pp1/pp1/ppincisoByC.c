#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

/*programa en paralelo que responde al inciso b*/

// Función para generar una matriz de tamaño 'size' con valores aleatorios
void generate_matrix(double *matrix, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i * size + j] = (double)(rand() % 100); // Asignar un valor aleatorio entre 0 y 99
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size; 
    double *initial_matrix = NULL; 
    int tamanomatriz, fila, columna; 

    MPI_Init(&argc, &argv); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        printf("\nIngrese el tamaño de la matriz: ");
        fflush(stdout); 
        scanf("%d", &tamanomatriz); 
    }

    // Difundir el tamaño de la matriz a todos los procesos
    MPI_Bcast(&tamanomatriz, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        initial_matrix = (double *)malloc(tamanomatriz * tamanomatriz * sizeof(double));
        srand(time(NULL)); 
        generate_matrix(initial_matrix, tamanomatriz); // Generar la matriz aleatoria
        printf("Matriz Cuadrada %dx%d:\n", tamanomatriz, tamanomatriz);
        
        for (fila = 0; fila < tamanomatriz; fila++) {
            for (columna = 0; columna < tamanomatriz; columna++) {
                printf("%6.1f ", initial_matrix[fila * tamanomatriz + columna]);
            }
            printf("\n");
        }
        
        fflush(stdout);
    }

    // Calcular cuántas filas asignar a cada proceso
    int filas_por_procesos = tamanomatriz / size;
    int remaining_filas = tamanomatriz % size;
    
    int *assigned_filas = (int *)malloc(size * sizeof(int)); 
    int *offsets = (int *)malloc(size * sizeof(int));
    
    for (int i = 0, current_position = 0; i < size; i++) {
        if (i < remaining_filas) {
            assigned_filas[i] = (filas_por_procesos + 1) * tamanomatriz; // Asignar filas adicionales si hay sobrantes
        } else {
            assigned_filas[i] = filas_por_procesos * tamanomatriz;
        }
        offsets[i] = current_position; // Calcular desplazamiento para cada proceso
        current_position += assigned_filas[i];
    }

    int local_rows = assigned_filas[rank] / tamanomatriz; // Filas locales asignadas al proceso actual
    double *local_matrix = (double *)malloc(assigned_filas[rank] * sizeof(double)); 

    double tiempoinicial = MPI_Wtime(); // Iniciar temporizador

    // Distribuir las filas de la matriz a los procesos correspondientes
    MPI_Scatterv(initial_matrix, 
                 assigned_filas, 
                 offsets, 
                 MPI_DOUBLE, 
                 local_matrix, 
                 assigned_filas[rank], 
                 MPI_DOUBLE, 
                 0, 
                 MPI_COMM_WORLD);

    if(local_rows > 0){  
     printf("\nFilas del proceso %d :\n", rank);
     for (fila = 0; fila < local_rows; fila++) {
       printf("Fila numero %d ", fila + offsets[rank] / tamanomatriz); // Mostrar filas asignadas al proceso actual
     }
    }
    
    printf("\n");
    fflush(stdout);
    
    double local_max = local_matrix[0]; 
    
    for (fila = 0; fila < local_rows; fila++) {
        for (columna = 0; columna < tamanomatriz; columna++) {
            double current_value = local_matrix[fila * tamanomatriz + columna];
            if (current_value > local_max) { 
                local_max = current_value;
            }
        }
    }

     if(local_rows > 0){  
       printf("El máximo en el proceso %d es: %.1f\n", rank, local_max);
     }
     
     fflush(stdout);
    
     double global_max = 0.0;
     
     // Reducir los máximos locales a un máximo global en todos los procesos (inciso b)
     MPI_Allreduce(&local_max, 
                   &global_max, 
                   1, 
                   MPI_DOUBLE, 
                   MPI_MAX, 
                   MPI_COMM_WORLD);

     double tiempofinal = MPI_Wtime(); // Finalizar temporizador
     double tiempoparalelo = tiempofinal - tiempoinicial; // Calcular tiempo total en segundos

     printf("Proceso %d: El máximo global recibido es: %.1f\n", rank, global_max);
     fflush(stdout);
     
     if (rank == 0) {
         printf("Tiempo de ejecución en paralelo %.6f segundos\n", tiempoparalelo);
         fflush(stdout);
         free(initial_matrix); // Liberar memoria asignada a la matriz inicial
     }
// Liberar memoria asignada 
     free(local_matrix); 
     free(assigned_filas);
     free(offsets); 

     MPI_Finalize(); 
     return 0; 
}
