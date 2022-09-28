add_module(
    ColorRandom
    "map index data to random colors"
    ../Color/Color.cpp
    ../Color/matplotlib.cpp
    ../Color/turbo_colormap.c
    ../Color/fake_parula.cpp
    ../Color/etopo1_modified.cpp)

target_compile_definitions(ColorRandom PRIVATE COLOR_RANDOM)

use_openmp()
