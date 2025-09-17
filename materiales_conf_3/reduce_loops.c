#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int i, rank, nprocs, count, start, stop, nloops, total_nloops;

    MPI_Init(&argc, &argv);

    // get the number of processes, and the id of this process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // we want to perform 1000 iterations in total. Work out the 
    // number of iterations to perform per process...
    count = 1000 / nprocs;

    // we use the rank of this process to work out which
    // iterations to perform.
    start = rank * count;
    stop = start + count;

    // now perform the loop
    nloops = 0;

    for (i=start; i<stop; ++i)
    {
        ++nloops;
    }

    printf("Process %d performed %d iterations of the loop.\n",
           rank, nloops);

    total_nloops = 0;
    MPI_Reduce( &nloops, &total_nloops, 1, MPI_INT, MPI_SUM,
                0, MPI_COMM_WORLD );

    if (rank == 0)
    {
        // ok - are there any more loops to run?
        nloops = 0;

        for (i=total_nloops; i<1000; ++i)
        {
            ++nloops;
        }

        printf("Process 0 performed the remaining %d iterations of the loop\n",
               nloops);
    }

    MPI_Finalize();

    return 0;
}