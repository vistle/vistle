add_module(
    Tracer
    "compute particle traces"
    Tracer.cpp
    BlockData.cpp
    Integrator.cpp
    Particle.cpp
    TracerTimes.cpp)

#use_openmp()
