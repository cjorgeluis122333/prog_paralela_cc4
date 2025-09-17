#include <mpi.h>
#include <stdio.h>

#define MCW MPI_COMM_WORLD

int main(int argc, char *argv[])
{
    int np, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MCW, &np);
    MPI_Comm_rank(MCW, &p);

    int dato_local = p + 1;  // cada proceso crea un valor distinto
    int datos_recolectados[np];  // buffer en el root para guardar todo

    // Recolecta "dato_local" de cada proceso en el array datos_recolectados
    MPI_Gather(&dato_local,      // buffer de envío (cada proceso)
               1,MPI_INT,       // cantidad y tipo
               datos_recolectados, // buffer de recepción (solo root)
               1, MPI_INT,       // cantidad y tipo recibidos de cada proceso
               0, MCW);          // root y comunicador

    // Solo el root imprime el resultado
    if (p == 0) {
        printf("Datos recolectados en el root:\n");
        for (int i = 0; i < np; i++) {
            printf("  del proceso %d: %d\n", i, datos_recolectados[i]);
        }
    }

    MPI_Finalize();
    return 0;
}
