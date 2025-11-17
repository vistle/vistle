# CPU module
add_module(VectorField "visualize vector field as line glyphs"
           VectorField.cpp VectorField.h)

# VTKm module
set(HEADERS
    VectorField.h
    filter/VectorFieldWorklet.h)

set(SOURCES
    VectorField.cpp)

add_vtkm_module(VectorFieldVtkm "visualize vector field as line glyphs (Viskores)"
                ${HEADERS} ${SOURCES})

# Use same source but different class name + macro for the VTKm version
target_compile_definitions(VectorFieldVtkm PRIVATE VectorField=VectorFieldVtkm)
target_compile_definitions(VectorFieldVtkm PRIVATE VECTORFIELDVTKM)


# Sources to be compiled with CUDA compiler
if(VISTLE_USE_CUDA AND VISTLE_USE_SEPARABLE_COMPILATION)
    viskores_add_target_information(
        vistle_viskores
        MODIFY_CUDA_FLAGS
        DEVICE_SOURCES ${SOURCES}
    )
endif()


