cmake_minimum_required(VERSION 3.5)
project(MC_MPn LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)

execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DVERSION=\"${GIT_COMMIT_HASH}\"")

#find_package(CUDA)
#list(APPEND CUDA_NVCC_FLAGS "-std=c++14;-O3;-DVERBOSE")
#set(CUDA_PROPAGATE_HOST_FLAGS OFF)

find_package(Armadillo REQUIRED)
find_package(BLAS REQUIRED)
find_path(BLAS_INCLUDE_DIRS cblas.h)

message(${BLAS_FOUND})
message(${BLAS_LIBRARIES})

option(Enable_MPI "Enables MPI Sampling" Off)
if (Enable_MPI)
    find_package(MPI REQUIRED)
endif()

option(NDEBUG "enables NDEBUG macro" Off)
if(${NDEBUG})
    add_definitions(-DNDEBUG)
endif()

option(DIMER_PRINT "enables printing of all integrands for dimer calculation macro" Off)
if(${DIMER_PRINT})
    add_definitions(-DDIMER_PRINT)
endif()

add_subdirectory(src)
