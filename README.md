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
ssh -p 2022 uclv_jcvidal@login.uclv.hpc.cu
