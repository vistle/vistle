add_module(BoundingBox "compute bounding boxes" BoundingBox.cpp)

target_compile_definitions(BoundingBox PRIVATE -DBOUNDINGBOX)
if(VISTLE_USE_OPENMP AND TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(BoundingBox PRIVATE OpenMP::OpenMP_CXX)
endif()
