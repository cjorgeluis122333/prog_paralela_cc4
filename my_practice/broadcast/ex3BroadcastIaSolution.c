//
// Created by cjorg on 10/5/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

// Estructura para parámetros físicos
typedef struct {
    double gravity;
    double spring_constant;
    double time_step;
    int total_steps;
    double domain_size[3];
} PhysicsParams;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ==================================================================
    // SITUACIÓN DONDE MPI_Bcast ES MUY EFICIENTE:
    // - Mismos parámetros para todos los procesos
    // - Datos relativamente pequeños pero críticos
    // - Comunicación one-to-all
    // ==================================================================

    PhysicsParams params;
    double simulation_time = 0.0;

    // Solo el proceso raíz inicializa los parámetros
    if(rank == 0)
    {
        // Parámetros de simulación física
        params.gravity = 9.81;
        params.spring_constant = 150.0;
        params.time_step = 0.001;
        params.total_steps = 1000;
        params.domain_size[0] = 10.0;  // x
        params.domain_size[1] = 10.0;  // y
        params.domain_size[2] = 10.0;  // z

        printf("=== INICIALIZACIÓN DE SIMULACIÓN FÍSICA ===\n");
        printf("Gravedad: %.2f m/s²\n", params.gravity);
        printf("Constante de resorte: %.1f N/m\n", params.spring_constant);
        printf("Paso de tiempo: %.3f s\n", params.time_step);
        printf("Pasos totales: %d\n", params.total_steps);
        printf("Tamaño del dominio: %.1fx%.1fx%.1f m\n",
               params.domain_size[0], params.domain_size[1], params.domain_size[2]);
        printf("Distribuyendo parámetros a %d procesos...\n\n", size);
    }//Salgo del proceso 0


    // ================================
    // ALTERNATIVA EFICIENTE: MPI_Bcast
    // ================================
    MPI_Bcast(&params, sizeof(PhysicsParams), MPI_BYTE, 0, MPI_COMM_WORLD);

    // ==================================================================
    // ¿POR QUÉ MPI_Bcast ES MÁS EFICIENTE?
    // 1. Optimización interna: MPI puede usar algoritmos como árbol binario
    // 2. Menos latencia: Una sola operación colectiva vs múltiples envíos
    // 3. Mejor uso de ancho de banda en redes especializadas
    // ==================================================================

    // Ahora todos los procesos tienen los mismos parámetros
    // Cada proceso simula un subconjunto diferente de partículas

    int particles_per_process = 1000;
    int start_particle = rank * particles_per_process;
    int end_particle = start_particle + particles_per_process;

    printf("Proceso %d: Simulando partículas %d a %d\n",
           rank, start_particle, end_particle-1);

    // Simulación local de partículas (cálculo intensivo)
    double local_kinetic_energy = 0.0;
    double local_potential_energy = 0.0;

    for(int step = 0; step < params.total_steps; step++)
    {
        // Cada proceso hace cálculos locales con los mismos parámetros globales
        for(int i = start_particle; i < end_particle; i++)
        {
            // Simulación física simplificada
            double velocity = 2.0 * sin(step * params.time_step + i * 0.1);
            double position = 5.0 * cos(step * params.time_step + i * 0.05);

            local_kinetic_energy += 0.5 * velocity * velocity;
            local_potential_energy += params.gravity * position +
                                      0.5 * params.spring_constant * position * position;
        }

        simulation_time += params.time_step;
    }

    // Reducir resultados de todos los procesos
    double total_kinetic, total_potential;
    MPI_Reduce(&local_kinetic_energy, &total_kinetic, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_potential_energy, &total_potential, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if(rank == 0)
    {
        printf("\n=== RESULTADOS DE LA SIMULACIÓN ===\n");
        printf("Tiempo total simulado: %.2f s\n", simulation_time);
        printf("Energía cinética total: %.2f J\n", total_kinetic);
        printf("Energía potencial total: %.2f J\n", total_potential);
        printf("Energía mecánica total: %.2f J\n", total_kinetic + total_potential);
    }

    // ==================================================================
    // COMPARACIÓN DE EFICIENCIA
    // ==================================================================
    if(rank == 0)
    {
        printf("\n=== ANÁLISIS DE EFICIENCIA ===\n");
        printf("MPI_Bcast vs MPI_Send/MPI_Recv para broadcast:\n");
        printf("- MPI_Bcast: 1 operación colectiva optimizada\n");
        printf("- MPI_Send/MPI_Recv: %d operaciones punto-a-punto\n", size-1);
        printf("- En redes de alta velocidad, MPI_Bcast puede ser %dx más rápido\n",
               (int)log2(size) + 1);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}