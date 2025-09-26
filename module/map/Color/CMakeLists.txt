add_module(Color "map scalar and vector data to colors" Color.cpp)

add_library(vistle_colormaps STATIC matplotlib.cpp turbo_colormap.c fake_parula.cpp etopo1_modified.cpp)
target_link_libraries(Color PRIVATE vistle_colormaps)

if(VISTLE_USE_OPENMP AND TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(Color PRIVATE OpenMP::OpenMP_CXX)
endif()
