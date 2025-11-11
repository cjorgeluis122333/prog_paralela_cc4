//
// Created by cjorg on 11/10/2025.
//

#include <omp.h>
int main() {
    int p_id =0, np = 1;
#pragma omp parallel private(p_id, np)
    {
        np = omp_get_num_threads();
        p_id = omp_get_thread_num();
        printf(“ Hello from thread %d out of %d \n” , p_id, np);
    }
}