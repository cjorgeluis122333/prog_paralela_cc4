#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void inicializarMatriz(int filas, int columnas, int matriz[filas][columnas]) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            matriz[i][j] = rand() % 100; // Inicializa con valores aleatorios entre 0 y 99
        }
    }
}

void calcularSumaFilas(int filas, int columnas, int matriz[filas][columnas], int suma[filas]) {
    for (int i = 0; i < filas; i++) {
        suma[i] = 0; // Inicializa la suma de la fila
        for (int j = 0; j < columnas; j++) {
            suma[i] += matriz[i][j]; // Suma los elementos de la fila
        }
    }
}

void imprimirMatriz(int filas, int columnas, int matriz[filas][columnas]) {
    printf("Matriz:\n");
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            printf("%3d ", matriz[i][j]);
        }
        printf("\n");
    }
}

void imprimirSumaFilas(int filas, int suma[filas]) {
    printf("Suma de cada fila:\n");
    for (int i = 0; i < filas; i++) {
        printf("Fila %d: %d\n", i + 1, suma[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <filas> <columnas>\n", argv[0]);
        return 1;
    }

    int filas = atoi(argv[1]);
    int columnas = atoi(argv[2]);

    // Verifica que las dimensiones sean positivas
    if (filas <= 0 || columnas <= 0) {
        printf("Las dimensiones deben ser mayores que cero.\n");
        return 1;
    }

    srand(time(NULL)); 

    int matriz[filas][columnas];
    int suma[filas];

    // Medir el tiempo de ejecución
    clock_t inicio = clock();

    inicializarMatriz(filas, columnas, matriz);
    calcularSumaFilas(filas, columnas, matriz, suma);
    
    clock_t fin = clock();
    
    imprimirMatriz(filas, columnas, matriz);
    imprimirSumaFilas(filas, suma);

    // Calcular y mostrar el tiempo transcurrido
    double tiempo_transcurrido = (double)(fin - inicio) / CLOCKS_PER_SEC;
    printf("Tiempo de ejecución: %.6f segundos\n", tiempo_transcurrido);

    return 0;
}
