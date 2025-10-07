#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI_REAL 3.141592653589793238462643 // Valor real de PI

int main(int argc, char *argv[]) {
    int pasos = 100000; // Número de pasos por defecto
    if (argc > 1) {
        pasos = atoi(argv[1]); // Longitud de pasos desde argumentos para cualquier valor de pasos 
    }
    
    double tamano_paso = 1.0 / (double)pasos; // Tamaño de cada paso
    double suma_total = 0.0; // Suma total para el cálculo de PI
    
    int rank, np;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Obtener el rango del proceso actual
    MPI_Comm_size(MPI_COMM_WORLD, &np); // Obtener el número total de procesos

    // Dividir el trabajo entre procesos
    int pasos_locales = pasos / np; // Número de pasos por proceso
    double suma_local = 0.0; // Suma local para cada proceso
    
    double start_time = MPI_Wtime();//iniciando tiempo
    // Calcular el producto en paralelo
    #pragma omp parallel 
    {
        double x; // Variable para almacenar el valor de x en cada iteración
        #pragma omp for reduction(+:suma_local)
        for (int i = rank * pasos_locales; i < (rank + 1) * pasos_locales; i++) {//asegura que cada proceso trabaje con su fragmento
            x = (i + 0.5) * tamano_paso; // Calcular x para el paso actual
            suma_local += 4.0 / (1.0 + x * x); // Sumar al total local
        }
    }
    double end_time = MPI_Wtime();//terminando tiempo

    // Reducir resultados locales a globales en el proceso 0
    MPI_Reduce(&suma_local, &suma_total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        double pi_aproximado = tamano_paso * suma_total; // Calcular PI aproximado
        printf("Valor aproximado de PI : \n%f\n", pi_aproximado);
        
        // Calcular el error respecto al valor real de PI
        double error = fabs(pi_aproximado - PI_REAL);
        printf("Error: \n%e\n", error);
        printf("Tiempo total de ejecución paralela: %f segundos\n", end_time - start_time);

    }

    MPI_Finalize(); // Finalizar MPI
    return 0;
}
