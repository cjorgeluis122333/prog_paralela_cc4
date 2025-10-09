//
// Created by cjorg on 10/7/2025.
//2)El root tiene un buffer de caracteres (p.ej. un “texto largo”).
// Lo reparte en segmentos de tamaño irregular a cada proceso.
// Cada proceso convierte a mayúsculas y se reconstruye el texto en root
//Usar: ScaterV y GatherV-> Porque no es divisible
//BrostCas

//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>

void to_uppercase(char *str, int length) {
    for (int i = 0; i < length; i++) {
        if(str[i] != '\0') {
            str[i] = toupper(str[i]);
        }
    }
}
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char *original_text = NULL;
    int total_length = 0;


    if(rank==0){
        // Texto de ejemplo - CORREGIDO: usar memoria dinámica
        char temp_text[] = "este es un texto de ejemplo que sera dividido en segmentos irregulares y convertido a mayusculas por diferentes procesos";
        //STRLEN: Is the quantity characters of the text
        total_length = strlen(temp_text) + 1; // +1 para el carácter nulo
        //Reservar memoria para la asignacion
        original_text = (char*)malloc(total_length * sizeof(char));
        //strcpy(Destino,Origen) Copia en destino el contenido de origen
        strcpy(original_text, temp_text);

        printf("Texto original: %s\n", original_text);
        printf("Longitud total: %d\n", total_length);
    }

    // Broadcast de la longitud total a todos los procesos
    MPI_Bcast(
            &total_length,   // Buffer de datos (envío/recepción)
            1,              // Cantidad de elementos (1 entero)
            MPI_INT,        // Tipo de dato (entero)
            0,              // Proceso root (rank 0):Identifica qué proceso es el emisor del broadcast.
            MPI_COMM_WORLD  // Comunicador (todos los procesos)
    );


    // ===========================================
    // PARTE 2: CALCULAR LA DISTRIBUCIÓN
    // ===========================================
    // Necesitamos calcular cuántos elementos le tocan a cada proceso
    int *sendcounts = NULL;  // Cuántos elementos manda a cada proceso
    int *displs = NULL;      // Desplazamientos para cada proceso

    if(rank == 0)
    {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));

        int base = total_length / size;     // Cantidad base para cada proceso
        int resto = total_length % size;    // Elementos sobrantes

        printf("Distribución: %d elementos, %d procesos\n", total_length, size);
        printf("Cada proceso recibe al menos %d elementos\n", base);
        printf("%d procesos reciben 1 elemento extra\n", resto);

        // Calcular sendcounts y displs
        int desplazamiento_actual = 0;
        for(int i = 0; i < size; i++)
        {
            sendcounts[i] = base;
            if(i < resto)  // Los primeros 'resto' procesos reciben un elemento extra
            {
                sendcounts[i]++;
            }
            displs[i] = desplazamiento_actual;
            desplazamiento_actual += sendcounts[i];

            printf("Proceso %d: recibe %d elementos, desplazamiento %d\n",
                   i, sendcounts[i], displs[i]);
        }
    }

    // Cada proceso necesita saber cuántos elementos va a recibir
    int mi_cantidad;

    // Distribuimos sendcounts para que cada proceso sepa cuántos elementos recibirá
    MPI_Scatter(sendcounts, 1, MPI_INT, &mi_cantidad, 1, MPI_INT, 0, MPI_COMM_WORLD);

    printf("Proceso %d: Voy a recibir %d elementos\n", rank, mi_cantidad);

    // Cada proceso crea un arreglo para recibir SU parte CORREGIDO: +1 para carácter nulo(Cuando se trabaja con texto)
    char *mi_parte = (char*)malloc((mi_cantidad + 1) * sizeof(char));

    // ===========================================
    // PARTE 3: DISTRIBUCIÓN CON MPI_SCATTERV
    // ===========================================
    MPI_Scatterv(
            original_text,      // Arreglo que se va a dividir
            sendcounts,          // Cuántos elementos para CADA proceso
            displs,              // Desplazamiento para CADA proceso
            MPI_CHAR,             // Tipo de dato
            mi_parte,            // Donde cada proceso recibe SU parte
            mi_cantidad,         // Cuántos elementos recibe ESTE proceso
            MPI_CHAR,             // Tipo de dato
            0,                   // Proceso raíz
            MPI_COMM_WORLD
    );


    // ===========================================
    // PARTE 4: PROCESAMIENTO LOCAL (Logica del programa)
    // ===========================================
    printf("Proceso %d: Recibí -> ", rank);
    printf("Proceso %d recibió: %s\n", rank, mi_parte);
    printf("\n");

    //Make Upper every character
    to_uppercase(mi_parte,mi_cantidad);




    // ===========================================
    // PARTE 5: RECOLECCIÓN CON MPI_GATHERV
    // ===========================================
    char *texto_final  = NULL;
    int *recvcounts = NULL;  // Para Gatherv también necesitamos recvcounts
    int *rdispls = NULL;     // y desplazamientos de recepción

    if(rank == 0)
    {
        texto_final  = (char*)malloc(total_length * sizeof(char));//Cantidad de elementos del arreglo
        recvcounts = (int*)malloc(size * sizeof(int));
        rdispls = (int*)malloc(size * sizeof(int));

        // Para Gatherv, recvcounts es igual a sendcounts
        // y rdispls es igual a displs
        for(int i = 0; i < size; i++)
        {
            recvcounts[i] = sendcounts[i];
            rdispls[i] = displs[i];
        }
    }

    MPI_Gatherv(
            mi_parte,                // Lo que cada proceso envía
            mi_cantidad,             // Cuántos elementos envía ESTE proceso
            MPI_CHAR,                 // Tipo de dato
            texto_final,    // Donde se reunen todos
            recvcounts,              // Cuántos elementos recibe de CADA proceso
            rdispls,                 // Desplazamientos para CADA proceso
            MPI_CHAR,                 // Tipo de dato
            0,                       // Proceso destino
            MPI_COMM_WORLD
    );



    // El proceso 0 muestra los resultados finales
    if(rank == 0)
    {
        // CORREGIDO: Asegurar que el texto final termine en null
        texto_final[total_length-1] = '\0';
        printf("\n=== RESULTADO FINAL ===\n");
        printf("Texto convertido a mayúsculas: %s\n", texto_final);

        // Liberar memoria - CORREGIDO
        free(original_text);  // Ahora sí es memoria dinámica
        free(texto_final);
        free(sendcounts);
        free(displs);
        free(recvcounts);
        free(rdispls);
    }

    free(mi_parte);

    MPI_Finalize();
    return 0;
}