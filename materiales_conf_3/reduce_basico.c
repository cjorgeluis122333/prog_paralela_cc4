#include <mpi.h>
#include <stdio.h>

#define MCW MPI_COMM_WORLD

int main(int argc, char *argv[])
{
    int np, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MCW, &np);
    MPI_Comm_rank(MCW, &p);

    int valor_local = p + 1;  // cada proceso tiene un número distinto
    int suma_total;           // el root recibirá la suma

    // Reducir todos los valores locales a una suma en el root (p=0)
    MPI_Reduce(&valor_local,    // valor de cada proceso
               &suma_total,     // resultado en root
               1, MPI_INT,      // cantidad y tipo
               MPI_SUM,         // operación (suma)
               0, MCW);         // root y comunicador

    // Solo el root imprime el resultado
    if (p == 0) {
        printf("Suma total de los procesos = %d\n", suma_total);
    }

    MPI_Finalize();
    return 0;
}
