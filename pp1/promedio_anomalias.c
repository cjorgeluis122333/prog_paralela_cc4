//
// Created by cjorg on 10/9/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>  // Para INT_MAX


int sumar_vector(int *vector, int length) {
    int suma = 0;
    for (int i = 0; i < length; i++) {
        suma += vector[i];
    }
    return suma;
}

int down_of_promedio(int *vector, double promedio, int length, int k) {
    int count = 0;
    for (int i = 0; i < length; i++) {
        if (vector[i] - promedio > k)
            count += 1;
    }
    return count;
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ===========================================
    // PARTE 1: LECTURA DEL Valor K
    // ===========================================
    int K = 0;  // Tamaño de la K

    if(rank == 0) {
        // Solicitar al usuario el tamaño de K
        printf("=== Ingrese el valor de K ===\n");

        if(argc > 1) {
            // Si se proporciona como argumento
            K = atoi(argv[1]);
            printf("Tamaño de K es:%d\n", K);
        } else {
            // Solicitar por entrada estándar
            printf("Ingrese el tamaño de K: ");
            scanf("%d", &K);
            printf("El valor de K es: %d\n", K);
        }

        // Validar que N sea positivo
        if(K <= 0) {
            printf("Error: El tamaño de la matriz debe ser positivo.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        printf("Número de procesos: %d\n\n", size);
    }

    // Distribuir el tamaño N a todos los procesos
    MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Validar que la matriz pueda ser distribuida
    if(K == 0) {
        MPI_Finalize();
        return 0;
    }



    int *vector_completo = NULL;
    int total_elementos = 0;

    //  Inicializacion del vector
    if (rank == 0) {
        // Configurar el tamaño del vector
        total_elementos = 1000;  // Podemos cambiar este valor

        // Crear y llenar el vector con valores aleatorios
        vector_completo = (int *) malloc(total_elementos * sizeof(int));
        srand(time(NULL));

        printf("=== ORDENAMIENTO DISTRIBUIDO DE VECTOR ===\n");
        printf("Tamaño del vector: %d elementos\n", total_elementos);
        printf("Número de procesos: %d\n", size);

        printf("Vector original: ");
        for (int i = 0; i < total_elementos; i++) {
            vector_completo[i] = rand() % 1000;  // Valores entre 0-999
            printf("%d ", vector_completo[i]);
        }
        printf("\n\n");
    }

    // Broadcast del tamaño total a todos los procesos
    MPI_Bcast(&total_elementos, 1, MPI_INT, 0, MPI_COMM_WORLD);


    // ===========================================
    // PARTE 2: DISTRIBUCIÓN DEL VECTOR
    // ===========================================
    int *sendcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        sendcounts = (int *) malloc(size * sizeof(int));
        displs = (int *) malloc(size * sizeof(int));

        // Calcular distribución de elementos
        int base = total_elementos / size;
        int resto = total_elementos % size;

        printf("Distribución de elementos:\n");
        int desplazamiento_actual = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = base;
            if (i < resto) {
                sendcounts[i]++;  // Los primeros procesos reciben un elemento extra
            }
            displs[i] = desplazamiento_actual;
            desplazamiento_actual += sendcounts[i];

            printf("  Proceso %d: %d elementos (desplazamiento: %d)\n",
                   i, sendcounts[i], displs[i]);
        }
        printf("\n");
    }

    // Cada proceso descubre cuántos elementos recibirá
    int mi_cantidad;
    MPI_Scatter(sendcounts, 1, MPI_INT, &mi_cantidad, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso prepara buffer para recibir su segmento
    int *mi_segmento = (int *) malloc(mi_cantidad * sizeof(int));

    // Distribuir los segmentos del vector
    MPI_Scatterv(
            vector_completo, sendcounts, displs, MPI_INT,
            mi_segmento, mi_cantidad, MPI_INT,
            0, MPI_COMM_WORLD
    );


    // ===========================================
    // PARTE 3: Promedio Local
    // ===========================================
    printf("Proceso %d: Recibí %d elementos -> ", rank, mi_cantidad);
    for (int i = 0; i < mi_cantidad; i++) {
        printf("%d ", mi_segmento[i]);
    }
    printf("\n");


    int sumaLocal = sumar_vector(mi_segmento, mi_cantidad);


    printf("Proceso %d: Tiene promedio: %d ", rank, sumaLocal);


    // ===========================================
    // PARTE 5: CÁLCULO DEL PROMEDIO TOTAL CON MPI_REDUCE
    // ===========================================
    int suma_total;

    // MPI_Reduce combina todas las sumas locales en una suma total en el proceso 0
    MPI_Reduce(
            &sumaLocal,            // Dato local a enviar (suma de cada proceso)
            &suma_total,            // Donde se almacena el resultado (solo en proceso 0)
            1,                      // Cantidad de elementos
            MPI_INT,                // Tipo de dato
            MPI_SUM,                // Operación: SUMA
            0,                      // Proceso que recibe el resultado
            MPI_COMM_WORLD          // Comunicador
    );




    // ===========================================
    // PARTE 6: VERIFICACIÓN Y RESULTADOS (solo proceso 0)
    // ===========================================
    if (rank == 0) {
        printf("\n=== RESULTADO FINAL ===\n");
        printf("Suma total de toda la matriz: %d\n", suma_total);

        // Verificación: calcular la suma directamente en el proceso 0
        int suma_verificacion = 0;
        for (int i = 0; i < total_elementos; i++) {
            suma_verificacion += vector_completo[i];
        }
//        double promedio = suma_verificacion/ total_elementos;
        printf("Verificación (cálculo directo en proceso 0): %d\n", suma_verificacion);
        printf("Suma total optenida: %d\n", suma_total);


        if (suma_total == suma_verificacion) {
            printf("✓ Los resultados coinciden correctamente.\n");
        } else {
            printf("✗ ERROR: Los resultados NO coinciden.\n");
        }

        double promedio = suma_total / total_elementos;
        printf("El promedio total es %f \n", promedio);

        // Contar total elementos por debajo del promedio
        int downP = down_of_p romedio(vector_completo, promedio, total_elementos,K);
        printf("Por debajo del promedio: %d\n", downP);


        // Liberar memoria
        free(vector_completo);
        free(sendcounts);
        free(displs);
    }

    // ===========================================
    // PARTE 7: LIMPIEZA
    // ===========================================
    free(mi_segmento);


    MPI_Finalize();
    return 0;
}