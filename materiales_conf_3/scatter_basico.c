#include <mpi.h>
#include <stdio.h>

#define MCW MPI_COMM_WORLD

int main(int argc, char *argv[])
{
    int np, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MCW, &np);
    MPI_Comm_rank(MCW, &p);

    int datos_a_repartir[np];   // solo el root necesita llenarlo
    int dato_local;             // cada proceso recibirá uno

    if (p == 0) {
        // El root inicializa un arreglo con valores
        for (int i = 0; i < np; i++) {
            datos_a_repartir[i] = (i + 1) * 10;
        }
        printf("Root tiene el arreglo: ");
        for (int i = 0; i < np; i++) printf("%d ", datos_a_repartir[i]);
        printf("\n");
    }

    // Distribuye un entero a cada proceso
    MPI_Scatter(datos_a_repartir, // buffer de envío (solo root)
                1, MPI_INT,       // cantidad y tipo que recibe cada proceso
                &dato_local,      // buffer de recepción (cada proceso)
                1, MPI_INT,
                0, MCW);          // root y comunicador

    // Cada proceso imprime el valor que recibió
    printf("Proceso %d recibió el valor %d\n", p, dato_local);

    MPI_Finalize();
    return 0;
}
