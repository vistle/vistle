add_library(vistle_colormaps STATIC matplotlib.cpp turbo_colormap.c fake_parula.cpp etopo1_modified.cpp)

add_module(Color "map scalar and vector data to colors" Color.cpp)
target_link_libraries(Color vistle_colormaps)

use_openmp()
