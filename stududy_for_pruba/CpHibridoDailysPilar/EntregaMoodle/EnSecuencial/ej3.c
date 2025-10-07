#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define PI_REAL 3.141592653589793238462643 // Valor real de PI

int main(int argc, char *argv[]) {
    int pasos = 100000; // Número de pasos por defecto
    if (argc > 1) {
        pasos = atoi(argv[1]); // Longitud de pasos desde argumentos para cualquier valor de pasos 
    }
    
    double tamano_paso = 1.0 / (double)pasos; // Tamaño de cada paso
    double suma_total = 0.0; // Suma total para el cálculo de PI

    double start_time = (double)clock(); // Iniciando tiempo

    // Calcular el producto en secuencial
    for (int i = 0; i < pasos; i++) {
        double x = (i + 0.5) * tamano_paso; // Calcular x para el paso actual
        suma_total += 4.0 / (1.0 + x * x); // Sumar al total
    }

    double pi_aproximado = tamano_paso * suma_total; // Calcular PI aproximado

    double end_time = clock(); // Terminar tiempo

    // Mostrar resultados
    printf("Valor aproximado de PI : \n%f\n", pi_aproximado);
    
    // Calcular el error respecto al valor real de PI
    double error = fabs(pi_aproximado - PI_REAL);
    printf("Error: \n%e\n", error);
    printf("Tiempo total de ejecución secuencial: %f segundos\n", (end_time - start_time) / CLOCKS_PER_SEC);

    return 0;
}
