cmake_minimum_required(VERSION 2.6)
project(vistle)

macro(USE_OPENMP)
    if(NOT APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgomp")
    endif()
endmacro(USE_OPENMP)


#find_package(MPI REQUIRED)
#find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} "-Wall -Wno-unknown-pragmas")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(core)
add_subdirectory(control)
add_subdirectory(vistle)
add_subdirectory(module)
