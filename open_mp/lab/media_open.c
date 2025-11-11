//
// Created by cjorg on 11/10/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>       // for clock_t, clock(), CLOCKS_PER_SEC
#include <unistd.h>     // for sleep()

double getTiempoParalelo() {
    double itime, ftime, exec_time;
    itime = omp_get_wtime();

// Required code for which execution time needs to be computed

    ftime = omp_get_wtime();
    exec_time = ftime - itime;
    printf("\n\nTime taken is %f", exec_time);
    return exec_time;
}


// función principal para encontrar el tiempo de ejecución de un programa en C
double getTiempoSecuencial() {
    // para almacenar el tiempo de ejecución del código
    double time_spent = 0.0;

    clock_t begin = clock();

    // hacer algunas cosas aquí
    sleep(3);

    clock_t end = clock();

    // calcula el tiempo transcurrido encontrando la diferencia (end - begin) y
    // dividiendo la diferencia por CLOCKS_PER_SEC para convertir a segundos
    time_spent += (double) (end - begin) / CLOCKS_PER_SEC;

    printf("The elapsed time is %f seconds", time_spent);

    return 0;
}


double speedUp() {

    double tiempoSecuencial = getTiempoSecuencial();
    double tiempoParalelo = getTiempoParalelo();

    double speedUp = tiempoSecuencial / tiempoParalelo;

    printf("The speed up is %f", speedUp);

    return speedUp;
};

int main() {
    int p = 4;
    int *array_completo = NULL;
    int total_numeros = 16; // Podemos cambiar este número fácilmente
    printf("Start\n");

    array_completo = (int *) malloc(total_numeros * sizeof(int));

    double media = 0;
    for (int i = 0; i < total_numeros; i++) {
         array_completo[i] = i;
        printf("%d, ", array_completo[i]);

    }

    omp_set_num_threads(p);
    printf("CALCULAR MEDIA \n");

    #pragma omp parallel for reduction(+:media)
    for (int i = 0; i < total_numeros; i++) {
        media += array_completo[i];
        printf("%f,", media);
    }
    printf("After for %f\n",media);
    media = media / total_numeros;

    printf("After media %f\n",media);

    speedUp();

    return 0;

};