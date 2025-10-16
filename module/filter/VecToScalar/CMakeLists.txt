add_module(VecToScalar "transform vector fields to scalar fields" VecToScalar.cpp VecToScalar.h)

<<<<<<< HEAD
add_vtkm_module(VecToScalarVtkm "transform vector fields to scalar fields" VecToScalar.cpp VecToScalar.h)
target_compile_definitions(VecToScalarVtkm PRIVATE VecToScalar=VecToScalarVtkm)
target_compile_definitions(VecToScalarVtkm PRIVATE VECTOSCALARVTKM)
=======
# Enable GPU path only if the bridge exists; else build CPU-only
if(TARGET vistle_viskores)
    target_link_libraries(VecToScalar PRIVATE vistle_viskores)

    if(TARGET viskores_filter_vector_analysis)
        target_link_libraries(VecToScalar PRIVATE viskores_filter_vector_analysis)
    endif()

    target_compile_definitions(VecToScalar PRIVATE VISTLE_WITH_VTKM=1)
else()
    target_compile_definitions(VecToScalar PRIVATE VISTLE_WITH_VTKM=0)
endif()
>>>>>>> 8d310160 (Precommit check)
