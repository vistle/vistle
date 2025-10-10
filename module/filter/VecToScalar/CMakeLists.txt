add_module(VecToScalar "transform vector fields to scalar fields" VecToScalar.cpp VecToScalar.h)

add_vtkm_module(VecToScalarVtkm "transform vector fields to scalar fields" VecToScalar.cpp VecToScalar.h)
target_compile_definitions(VecToScalarVtkm PRIVATE VecToScalar=VecToScalarVtkm)
target_compile_definitions(VecToScalarVtkm PRIVATE VECTOSCALARVTKM)
