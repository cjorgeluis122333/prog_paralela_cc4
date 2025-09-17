#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MCW MPI_COMM_WORLD

int main(int argc, char *argv[]) {
    int np, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MCW, &np);
    MPI_Comm_rank(MCW, &p);

    int *datos = NULL;     // solo root lo usará
    int *result = NULL;    // solo root lo usará
    int local;             // cada proceso recibirá 1 entero

    if (p == 0) {
        // Reservar e inicializar arreglo de entrada (tamaño = np)
        datos = (int*)malloc(np * sizeof(int));
        result = (int*)malloc(np * sizeof(int));
        for (int i = 0; i < np; i++) datos[i] = (i + 1) * 10;  // 10, 20, 30, ...
        printf("Root: arreglo inicial: ");
        for (int i = 0; i < np; i++) printf("%d ", datos[i]);
        printf("\n");
    }

    // Repartir 1 entero a cada proceso
    MPI_Scatter(
        datos, 1, MPI_INT,   // buffer y tipo en root
        &local, 1, MPI_INT,  // buffer y tipo en cada proceso
        0, MCW
    );

    // --- Cómputo local: ejemplo simple ---
    // cada proceso multiplica su valor por (rank + 1)
    int local_result = local * (p + 1);

    // Recolectar los resultados en root
    MPI_Gather(
        &local_result, 1, MPI_INT,  // buffer local
        result,        1, MPI_INT,  // buffer en root
        0, MCW
    );

    if (p == 0) {
        printf("Root: resultados recolectados:\n");
        for (int i = 0; i < np; i++) {
            // ejemplo: para np=4 y datos=10,20,30,40
            // resultados = 10*1, 20*2, 30*3, 40*4 => 10,40,90,160
            printf("  del proceso %d: %d\n", i, result[i]);
        }
        free(datos);
        free(result);
    }

    MPI_Finalize();
    return 0;
}
