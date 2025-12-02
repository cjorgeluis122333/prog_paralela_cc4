# How work with MPI

Here you can found a lot of resolved samples of MPI in language C.
And the command for execute used linux or WSL2.
I hope that can help you

## Comandos Importantes Para MPI

  ``` MPI-Compilation
    //Para compilar un programa en MPI 
    mpicc ex3BroadcastIaSolution.c -o ex3Broadcast
    
    //Para compilar un programa en MPI utilizando funciones matematicas como Seno y Coseno se utiliza: -lm 
    mpicc ex3BroadcastIaSolution.c -o ex3Broadcast -lm


  ```

  ``` MPI-RUNING
    //Para ejecutar un programa en MPI 
    mpirun -np 2 ./ex3Broadcast
  ```

## Conectar al cluster 
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

## Paso 3 Compilar
```shell
mpicc -fopenmp -o ejer1 Ejer1.c -lm
```

## Compilar

#### Para la mayoria de los casos
```shell
 gcc -o media_omp media_open.c -fopenmp
```

#### Para funciones matematicas
```shell
 gcc -fopenmp -o ejer3 Ejercicio3.c -lm
```
## Ejecutar
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