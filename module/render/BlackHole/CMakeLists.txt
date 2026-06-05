add_module(BlackHole "BlackHole data sink" BlackHole.cpp)

target_link_libraries(BlackHole PRIVATE vistle_renderer vistle_util)
