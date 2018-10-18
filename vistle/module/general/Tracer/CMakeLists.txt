#vistle_find_package(Boost 1.53 REQUIRED COMPONENTS mpi)

add_module(Tracer Tracer.cpp BlockData.cpp Integrator.cpp Particle.cpp TracerTimes.cpp)

#use_openmp()
