//
// Created by cjorg on 10/5/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    // Verificar que se ejecuten exactamente 3 procesos
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(size != 3)
    {
        printf("Esta aplicación debe ejecutarse con 3 procesos.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Obtener el rango (identificador) del proceso actual
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(my_rank == 0)
    {
        // PROCESO 0: Envía el arreglo al proceso 2
        int array[5] = {2, 4, 6, 8, 10};
        printf("Proceso 0: Enviando arreglo [2,4,6,8,10] al proceso 2.\n");
        MPI_Send(array, 5, MPI_INT, 2, 0, MPI_COMM_WORLD);

        // Recibe el resultado del proceso 2
        int suma;
        MPI_Recv(&suma, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Proceso 0: Recibí la suma total del proceso 2: %d\n", suma);
    }
    else if(my_rank == 1)
    {
        // PROCESO 1: No participa en la comunicación
        printf("Proceso 1: No participo en esta operación.\n");
    }
    else if(my_rank == 2)
    {
        // PROCESO 2: Recibe el arreglo del proceso 0
        int array_recibido[5];
        MPI_Recv(array_recibido, 5, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Proceso 2: Recibí el arreglo del proceso 0.\n");

        // Calcula la suma de los elementos
        int suma = 0;
        for(int i = 0; i < 5; i++)
        {
            suma += array_recibido[i];
        }
        printf("Proceso 2: La suma de los elementos es %d. Enviando resultado al proceso 0.\n", suma);

        // Envía el resultado al proceso 0
        MPI_Send(&suma, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}