cmake_minimum_required(VERSION 3.27)
project(prog_paralela C)

set(CMAKE_C_STANDARD 11)

add_library(prog_paralela STATIC library.c
        calculate_total_sum.c
        materiales_conf_3/send_receive.c)
