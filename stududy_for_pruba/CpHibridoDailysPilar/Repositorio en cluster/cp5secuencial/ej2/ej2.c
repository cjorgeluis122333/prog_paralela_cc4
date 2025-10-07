#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void inicializar_vectores(double *vector_a, double *vector_b, int longitud) {
    for (int i = 0; i < longitud; i++) {
        vector_a[i] = rand() % 10; // Valores aleatorios para el vector a
        vector_b[i] = rand() % 10; // Valores aleatorios para el vector b
        printf("vector 1 posicion %d: %2.1f        ", i, vector_a[i]);
        printf("vector 2 posicion %d: %2.1f ", i, vector_b[i]);
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <longitud_vectores>\n", argv[0]);
        return 1;
    }

    int longitud_vectores = atoi(argv[1]); // Longitud de los vectores
    double *vector_a = malloc(longitud_vectores * sizeof(double));
    double *vector_b = malloc(longitud_vectores * sizeof(double));
    double suma_global = 0.0;

    if (vector_a == NULL || vector_b == NULL) {
        printf("Error al reservar memoria.\n");
        return 1;
    }

    srand(time(NULL)); // Inicializar la semilla para números aleatorios
    inicializar_vectores(vector_a, vector_b, longitud_vectores);

    double start_time = clock(); // Iniciar tiempo

    // Calcular producto escalar
    for (int i = 0; i < longitud_vectores; i++) {
        suma_global += vector_a[i] * vector_b[i]; // Sumar producto escalar
    }

    double end_time = clock(); // Terminar tiempo

    // Mostrar resultados
    printf("Producto escalar: %6.1f \n", suma_global);
    printf("Tiempo total de ejecución secuencial: %f segundos\n", (end_time - start_time) / CLOCKS_PER_SEC);

    free(vector_a);
    free(vector_b);

    return 0;
}
