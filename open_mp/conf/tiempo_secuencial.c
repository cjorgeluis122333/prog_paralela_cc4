#include <stdio.h>
#include <time.h>       // for clock_t, clock(), CLOCKS_PER_SEC
#include <unistd.h>     // for sleep()
 
// función principal para encontrar el tiempo de ejecución de un programa en C
int main()
{
    // para almacenar el tiempo de ejecución del código
    double time_spent = 0.0;
 
    clock_t begin = clock();
 
    // hacer algunas cosas aquí
    sleep(3);
 
    clock_t end = clock();
 
    // calcula el tiempo transcurrido encontrando la diferencia (end - begin) y
    // dividiendo la diferencia por CLOCKS_PER_SEC para convertir a segundos
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
 
    printf("The elapsed time is %f seconds", time_spent);
 
    return 0;
}
