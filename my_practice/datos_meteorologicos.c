//
// Created by cjorg on 10/7/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <math.h>

#define DIAS_SEMANA 7

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ===========================================
    // PARTE 1: CONFIGURACIÓN INICIAL CON MPI_BCAST
    // ===========================================
    int total_semanas = 0;

    if(rank == 0)
    {
        // El proceso coordinador define cuántas semanas analizar
        total_semanas = 4;  // Analizar 4 semanas de datos
        printf("=== ANÁLISIS METEOROLÓGICO DISTRIBUIDO ===\n");
        printf("Configuración: %d estaciones (procesos), %d semanas de datos\n\n",
               size, total_semanas);
    }

    // Todos los procesos necesitan saber cuántas semanas analizar
    MPI_Bcast(&total_semanas, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // ===========================================
    // PARTE 2: GENERACIÓN DE DATOS LOCALES
    // ===========================================
    // Cada estación (proceso) genera sus propias mediciones
    srand(time(NULL) + rank);  // Semilla diferente para cada proceso

    float **datos_locales = (float**)malloc(total_semanas * sizeof(float*));
    float suma_local = 0.0;
    float max_local = -100.0;
    float min_local = 100.0;
    int total_mediciones = 0;

    printf("Estación %d: Generando datos para %d semanas...\n", rank, total_semanas);

    for(int semana = 0; semana < total_semanas; semana++)
    {
        datos_locales[semana] = (float*)malloc(DIAS_SEMANA * sizeof(float));

        for(int dia = 0; dia < DIAS_SEMANA; dia++)
        {
            // Simular temperaturas realistas (entre 5°C y 35°C)
            datos_locales[semana][dia] = 5.0 + (rand() % 300) / 10.0;
            suma_local += datos_locales[semana][dia];
            total_mediciones++;

            if(datos_locales[semana][dia] > max_local)
                max_local = datos_locales[semana][dia];
            if(datos_locales[semana][dia] < min_local)
                min_local = datos_locales[semana][dia];
        }
    }

    float promedio_local = suma_local / total_mediciones;
    printf("Estación %d: Temperatura promedio local = %.2f°C (Max: %.2f, Min: %.2f)\n",
           rank, promedio_local, max_local, min_local);

    // ===========================================
    // PARTE 3: CÁLCULO DE ESTADÍSTICAS GLOBALES CON MPI_REDUCE
    // ===========================================
    // Usamos MPI_Reduce porque necesitamos OPERAR sobre los datos, no solo juntarlos

    float suma_global, promedio_global, max_global, min_global;

    MPI_Reduce(&suma_local, &suma_global, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&max_local, &max_global, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&min_local, &min_global, 1, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);

    // También necesitamos el total de mediciones para el promedio
    int total_mediciones_global;
    MPI_Reduce(&total_mediciones, &total_mediciones_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // ===========================================
    // PARTE 4: COMPARTIR DATOS INTERESANTES CON MPI_GATHER
    // ===========================================
    // Usamos MPI_Gather porque queremos todos los promedios locales, no operar sobre ellos

    float *promedios_locales = NULL;
    if(rank == 0)
    {
        promedios_locales = (float*)malloc(size * sizeof(float));
    }

    MPI_Gather(&promedio_local, 1, MPI_FLOAT,
               promedios_locales, 1, MPI_FLOAT,
               0, MPI_COMM_WORLD);

    // ===========================================
    // PARTE 5: PRESENTACIÓN DE RESULTADOS
    // ===========================================
    if(rank == 0)
    {
        promedio_global = suma_global / total_mediciones_global;

        printf("\n=== RESULTADOS GLOBALES ===\n");
        printf("Temperatura promedio global: %.2f°C\n", promedio_global);
        printf("Temperatura máxima registrada: %.2f°C\n", max_global);
        printf("Temperatura mínima registrada: %.2f°C\n", min_global);
        printf("Total de mediciones analizadas: %d\n", total_mediciones_global);

        printf("\n=== COMPARACIÓN ENTRE ESTACIONES ===\n");
        for(int i = 0; i < size; i++)
        {
            printf("Estación %d: promedio = %.2f°C", i, promedios_locales[i]);

            // Destacar la estación con promedio más cercano al global
            float diferencia = fabs(promedios_locales[i] - promedio_global);
            if(diferencia < 0.5)
            {
                printf(" (más representativa)");
            }
            printf("\n");
        }

        // Análisis de variabilidad
        printf("\n=== ANÁLISIS DE VARIABILIDAD ===\n");
        float suma_diferencias = 0.0;
        for(int i = 0; i < size; i++)
        {
            suma_diferencias += fabs(promedios_locales[i] - promedio_global);
        }
        float variabilidad_promedio = suma_diferencias / size;
        printf("Variabilidad promedio entre estaciones: %.2f°C\n", variabilidad_promedio);

        if(variabilidad_promedio < 2.0)
        {
            printf("Las estaciones muestran lecturas consistentes\n");
        }
        else
        {
            printf("Hay significativa variación entre estaciones\n");
        }

        free(promedios_locales);
    }

    // ===========================================
    // PARTE 6: LIMPIEZA
    // ===========================================
    for(int semana = 0; semana < total_semanas; semana++)
    {
        free(datos_locales[semana]);
    }
    free(datos_locales);

    MPI_Finalize();
    return 0;
}