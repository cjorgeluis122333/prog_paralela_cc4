/**
 * In this example i have two process. The process one
 */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    // Size of the default communicator
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);  //Cantidad de procesos que se estan ejecutando

    // Get my rank and do the corresponding job
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);  //Saber cual es el proceso que esta ejecutando esta parte de el codigo


    if (my_rank == 0) {// I am the process 0
        // Proceso 0: envía un arreglo de 5 elementos al proceso 2
        int buffer[5] = {2, 4, 6, 8, 10};
        MPI_Send(
                buffer,  //Value to pass
                5, //Count
                MPI_INT, // Tipo de dato
                1, //Destino
                0, //Etiqueta
                MPI_COMM_WORLD //MPI_Comm
        );
        printf("Process 0 sent array to process 2.\n");

        // Luego, recibe el resultado de la suma del proceso 2
        int sum;
        MPI_Recv(
                &sum, //received_message
                1, //count
                MPI_INT, //datatype
                1, //Origen process sender
                0, //tag
                MPI_COMM_WORLD, //MPI_Comm
                MPI_STATUS_IGNORE //MPI_Status
        );
        printf("Process 0 received sum from process 2: %d\n", sum);
    } else if (my_rank == 1){

        int array_recibido[5];
        MPI_Status status;

        MPI_Recv(array_recibido, 5, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        // Cantidad de elementos que recive del proceso 0
        int count;
        MPI_Get_count(&status, MPI_INT, &count);
        printf("La cantidad de elementos del areglo es %d",count);

        int sum = 0;
        for (int i = 0; i < count; ++i) {
           sum += array_recibido[i];
        }
        printf("La suma total es: %d",sum);

        //Send the sum to the process 0
        MPI_Send(
                &sum,  //Value to pass
                1, //Count
                MPI_INT, // Tipo de dato
                0, //Destino
                0, //Etiqueta
                MPI_COMM_WORLD //MPI_Comm
        );

    }


    MPI_Finalize();

    return EXIT_SUCCESS;

}