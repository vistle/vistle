add_module(
    Color
    "map scalar and vector data to colors"
    Color.cpp
    matplotlib.cpp
    turbo_colormap.c
    fake_parula.cpp
    etopo1_modified.cpp)

use_openmp()
