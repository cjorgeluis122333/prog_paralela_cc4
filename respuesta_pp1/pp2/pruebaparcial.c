#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

/*
El codigo implementado lo comente lo mas que pude para ser lo mas explicativa posible
Muestro por pantalla la mayoria de los resultados generados para mejor comprension
*/

//funcion que calcula norma infinita, en este caso de un conjunto de filas representados por una matriz unidimensional
double infNorm(double *A, int filas, int columnas) {
    double s, norm = 0.0;
    for (int i = 0; i < filas; i++) {
        s = 0.0;
        for (int j = 0; j < columnas; j++) {
            s += fabs(A[i * columnas + j]); // Acceso a elemento en matriz unidimensional
        }
        if (s > norm) {
            norm = s;
        }
    }
    return norm;
}

int main(int argc, char *argv[]) {
    int rank, size;
    double *matriz = NULL;
    int N, i, j;
//iniciar entorno MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {//el proceso raiz pide entrada del usuario(tamano de la matriz cuadrada)
        printf("Introduzca el tamaño de la matriz: \n ");
        fflush(stdout);
        scanf("%d", &N);
    }
//uso de Bcast para difundir el tamano de la matriz al resto de los procesos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {//generar matriz con valores aleatorios de 0 a 9
        matriz = (double *)malloc(N * N * sizeof(double));
        srand(time(NULL));
        printf("Matriz de %dx%d:\n",N,N);
        for (i = 0; i < N; i++) {
           printf("\n"); 
           for (j = 0; j < N; j++) {
                matriz[i * N + j] = (double)(rand() % 10);
                printf("%6.1f ", matriz[i * N + j]);
            }
            
        }
        printf("\n"); 
        fflush(stdout);
    }
    int *filasasignadas = (int *)malloc(size * sizeof(int));//almacena cuantas filas recibiria cada proceso
    int *desplazamiento = (int *)malloc(size * sizeof(int));//almacena desplazamiento para cada proceso (indice inicial de sus respectivos datos)

//calculo de filasasignadas y desplazamiento
    int contarfilas = 0;
    for (i = 0; i < size; i++) {
        filasasignadas[i] = (i < N % size) ? ((N / size) + 1) * N : (N / size) * N;
        //  N / size para saber la distribucion a hacer por proceso
        //  N % size para que en caso de no ser las filas divisible entre los procesos se halla el numero de filas que deben ser distribuidas entre los procesos
        desplazamiento[i] = contarfilas;
        contarfilas += filasasignadas[i];
    }

    double *matrizlocal = (double *)malloc(filasasignadas[rank] * sizeof(double)); //matriz unidimencional que guardara filas correspondientes del proceso

//uso de Scatterv para la distribucion de las filas de la matriz a los procesos
    MPI_Scatterv(matriz, filasasignadas, desplazamiento, MPI_DOUBLE, matrizlocal, filasasignadas[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

//para saber que filas recibio cada proceso
    printf("\nProceso %d recibió las siguientes filas:\n", rank);
    if(filasasignadas[rank]==0){
/*en caso de que la matriz sea de menor tamano que la cantidad de procesos, 
por lo que no se le asignarian filas a algun que otro proceso ya que no seria necesario*/
        printf("Al proceso %d no se le asignaron filas de la matriz\n",rank);
    }else{
        for (i = 0; i < filasasignadas[rank] / N; i++) {
        printf("Fila %d: ", i + desplazamiento[rank] / N);
        for (j = 0; j < N; j++) {
            int posicion=i * N + j;
            printf("%6.1f ", matrizlocal[posicion]);
        }
        printf("\n");
    }
    fflush(stdout);
    }
    

//obtener norma infinita de las filas del proceso actual
    double maximolocal = infNorm(matrizlocal, filasasignadas[rank] / N, N);
    printf("Norma infinita local en proceso %d: %3.1f\n", rank, maximolocal);

//uso de reduce para obtener el maximo de las normas infinitas locales que seria la norma infinita global mediante  la operacion MPI_MAX
    double maximoglobal = 0.0;
    MPI_Reduce(&maximolocal, &maximoglobal, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

//el proceso raiz imprime la norma infinita y libera espacio en memoria de lamatriz
    if (rank == 0) {
        printf("\n\t****La norma infinita de la matriz es: %6.1f ****\n", maximoglobal);
        free(matriz);
    }

//liberar espacio en memoria de arreglos
    free(matrizlocal);
    free(filasasignadas);
    free(desplazamiento);

//finalizar MPI
    MPI_Finalize();

    return 0;
}
