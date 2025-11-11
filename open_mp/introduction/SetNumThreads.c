/*
A continuación, se muestra un programa que emplea la biblioteca OpenMP para
inicializar un arreglo de 20 elementos enteros. En su implementación se solicita el
número de hilos que se desean emplear (el hilo maestro, y los hilos esclavos) y cada
uno de ellos inicializa la posición i correspondiente, con su rango o id.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define ARRAY_LENGTH 20

int main(int argc, char **argv) {
    int tid, nthr; // identificador del thread ynumero de threads
     int i, A[ARRAY_LENGTH];
    printf("\nIntroduce el numero de threads ---> ");
    scanf("%d", &nthr);
#ifdef _OPENMP
    omp_set_num_threads(nthr);
#endif
#pragma omp parallel for schedule(static, 2) private(i, tid)
    for (i = 0; i < ARRAY_LENGTH; i++) {
        tid = omp_get_thread_num(); //Know the thread id
        printf("Thread %d of %d is running\n", tid, nthr);
        A[i] = tid;
        printf("Thread %d has finished\n", tid);
    }
    for (i = 0; i < ARRAY_LENGTH; i++)
        printf("A(%d) = %d \n", i, A[i]);
    return (0);
}