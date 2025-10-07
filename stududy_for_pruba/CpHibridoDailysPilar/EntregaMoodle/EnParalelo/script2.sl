#!/bin/bash 
#SBATCH -p public
#SBATCH -J MPItest 
#SBATCH -o MPItest-%j.out 
#SBATCH -e MPItest-%j.err 
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1
#SBATCH --mem-per-cpu=4GB
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --mail-user=daoliver@uclv.cu

module load OpenMPI

mpirun -np 2 ej2 500