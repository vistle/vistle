#vistle_find_package(Boost 1.40 REQUIRED COMPONENTS mpi)

add_module(Tracer Tracer.cpp BlockData.cpp Integrator.cpp Particle.cpp TracerTimes.cpp)

#use_openmp()

include_directories(
        ../../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(Tracer
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
