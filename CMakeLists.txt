cmake_minimum_required(VERSION 3.18)
project(mi_proyecto_cuda LANGUAGES CXX CUDA)

# Configurar CUDA
enable_language(CUDA)

# Versión mínima de CUDA requerida
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Arquitecturas de GPU (ajusta según tu GPU)
set(CUDA_ARCHITECTURES "75;80;86;89")

# Flags de compilación
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -std=c++14 --expt-relaxed-constexpr -O2")
if(WIN32)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler=\"/W3\"")
endif()

# Crear ejecutable
add_executable(${PROJECT_NAME}
        src/main.cu
        src/kernels/vector_ops.cu
        src/kernels/matrix_ops.cu
)

# Incluir directorios
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enlazar librerías de CUDA
target_link_libraries(${PROJECT_NAME} CUDA::cudart)

# Propiedades del target
set_target_properties(${PROJECT_NAME} PROPERTIES
        CUDA_RUNTIME_LIBRARY Shared
        CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}"
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
)

#cmake_minimum_required(VERSION 3.27)
#project(prog_paralela C)
#
#set(CMAKE_C_STANDARD 11)
#
#add_library(prog_paralela STATIC library.c
#        calculate_total_sum.c
#        materiales_conf_3/send_receive.c)
