#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Función para generar una matriz
void generarMatriz(double **matriz, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            matriz[i][j] = rand() % 100; // Asignar un valor aleatorio entre 0 y 99
        }
    }
}

int main(int argc, char *argv[]) {
    double **initial_matrix = NULL; 
    int tamanomatriz; 
    printf("\nIngrese el tamaño de la matriz: ");
    fflush(stdout);
    scanf("%d", &tamanomatriz); 

    // Asignar memoria para la matriz
    initial_matrix = (double **)malloc(tamanomatriz * sizeof(double *));
    for (int i = 0; i < tamanomatriz; i++) {
        initial_matrix[i] = (double *)malloc(tamanomatriz * sizeof(double));
    }

    // Generar la matriz
    generarMatriz(initial_matrix, tamanomatriz); 

    printf("\nMatriz Cuadrada %dx%d:\n", tamanomatriz, tamanomatriz);
    
    for (int i = 0; i < tamanomatriz; i++) {
        for (int j = 0; j < tamanomatriz; j++) {
            printf("%6.1f ", initial_matrix[i][j]); 
        }
        printf("\n"); 
    }
    
    fflush(stdout); 
    clock_t tiempoinicial = clock(); // Iniciar cronómetro

    // Calcular el valor máximo en la matriz
    double valormaximo = initial_matrix[0][0]; // Inicializar con el primer elemento
    for (int i = 0; i < tamanomatriz; i++) {
        for (int j = 0; j < tamanomatriz; j++) {
            double current_value = initial_matrix[i][j];
            if (current_value > valormaximo) { // Comparar y actualizar el valor máximo
                valormaximo = current_value;
            }
        }
    }
    
    clock_t tiempofinal = clock(); // Detener cronómetro
    
    printf("El valor máximo es: %.1f\n", valormaximo); // Mostrar el valor máximo
    
    double tiempo_total_ms = (double)(tiempofinal - tiempoinicial) / CLOCKS_PER_SEC; // Calcular tiempo total en segundos
    printf("Tiempo de ejecución en secuencial: %.6f segundos\n", tiempo_total_ms);

    // Liberar memoria asignada para la matriz
    for (int i = 0; i < tamanomatriz; i++) {
        free(initial_matrix[i]); // Liberar cada fila
    }
    
    free(initial_matrix); // Liberar puntero a las filas

    return 0; 
}
