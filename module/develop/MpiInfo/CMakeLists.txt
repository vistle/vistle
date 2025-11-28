add_module(MpiInfo "print MPI, CPU binding and shared memory info" MpiInfo.cpp)

add_vtkm_module(MpiInfoVtkm "print MPI, Viskores/VTK-m, CPU binding and shared memory info" MpiInfo.cpp)
target_compile_definitions(MpiInfoVtkm PRIVATE MpiInfo=MpiInfoVtkm)
target_compile_definitions(MpiInfoVtkm PRIVATE MPIINFOVTKM)
