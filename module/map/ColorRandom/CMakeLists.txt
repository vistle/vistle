add_module(ColorRandom "map index data to random colors" ../Color/Color.cpp)

target_compile_definitions(ColorRandom PRIVATE COLOR_RANDOM)
target_link_libraries(ColorRandom vistle_colormaps)

use_openmp()
