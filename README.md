# How work with MPI

Here you can found a lot of resolved samples of MPI in language C.
And the command for execute used linux or WSL2.
I hope that can help you
##  ============================ MPI
## Important command for mpi

  ``` bash
    //Para compilar un programa en MPI 
    mpicc ex3BroadcastIaSolution.c -o ex3Broadcast
    
    //Para compilar un programa en MPI utilizando funciones matematicas como Seno y Coseno se utiliza: -lm 
    mpicc ex3BroadcastIaSolution.c -o ex3Broadcast -lm
  ```

  ``` bash
    //Para ejecutar un programa en MPI 
    mpirun -np 2 ./ex3Broadcast
  ```
##  ============================CUSTER

## Connect and execute a project in the HPC (CUSTER)

### Cuenta

ssh -p 2022 uclv_jcvidal@login.uclv.hpc.cu

### Paso 1: Configurar consola

Para definir el tipo de usuario y la particion

```shell
srun --account=UCLV -p gpu --gres=gpu:1 --pty /bin/bash
```

### Paso 2: Cargar ek Modulo

```shell
module load OpenMPI
```

### Paso 3 Compilar

```shell
mpicc -fopenmp -o ejer1 Ejer1.c -lm
```

## =================================CREATE SCRIPT


### Body Script

```

module load OpenMPI;

export OMP_NUM_THREADS = 16;

mpirun -np 1 ./ejer2;
```

### Content Script
```
#!/bin/sh 

### General options ### 
#SBATCH --job-name=job2
#SBATCH --export=ALL
#SBATCH --partition=gpu

### Resource handling ###
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=16
#SBATCH --mem=64G
#SBATCH --time=5-23:59:59

### Stream options ###
#SBATCH --output=ejer2Output
#SBATCH --error=ejerTwoError

### Bash script ###
module load OpenMPI;

export OMP_NUM_THREADS = 16;

mpirun -np 1 ./ejer2;
exit 0
```

## Execute Script
### Step 1
```bash
srun --account=UCLV -p gpu --gres=gpu:1 --pty /bin/bash
```
### Step 2 Avilitar Prmisos
Change the name of the Script
```shell
chmod 755 script.sh
```
### Step 3 Execute
```
./scriptEjer2.sh
```

##  ============================ OPEN-MP
## Compiled OMP-File

### In the most case

```shell
 gcc -o media_omp media_open.c -fopenmp
```

### For mathematics functions use -lm 

```shell
 gcc -fopenmp -o ejer3 Ejercicio3.c -lm
```

## Execute 

```shell
 ./media_omp
```

### Para compilar programas con computación híbrida usa de OpenMP y MPI

```bash
mpicc -fopenmp -o ejercicio1 Ejercicio1.c
```

# Para algunos casos

mpicc -fopenmp -o ejer2 Ejercicio2-1.c -lm

# Ejecutar con 4 procesos MPI

mpirun -np 4 ./ejer2 1000000