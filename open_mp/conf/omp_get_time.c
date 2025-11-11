#include <stdio.h>
#include <omp.h>
int main(){ 
  double itime, ftime, exec_time; 
  itime = omp_get_wtime(); 
  
// Required code for which execution time needs to be computed 
  
  ftime = omp_get_wtime(); 
  exec_time = ftime - itime; 
  printf("\n\nTime taken is %f", exec_time);
  return 0;
}

