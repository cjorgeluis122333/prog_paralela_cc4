//
// Created by cjorg on 10/7/2025.
//2)El root tiene un buffer de caracteres (p.ej. un “texto largo”).
// Lo reparte en segmentos de tamaño irregular a cada proceso.
// Cada proceso convierte a mayúsculas y se reconstruye el texto en root
//Usar: ScaterV y GatherV-> Porque no es divisible
//BrostCas

//

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char *a = "Texto de prueba";
    printf("Proceso %d: Voy a recibir %d \n", rank, a[1]);

    MPI_Finalize();
    return 0;
}