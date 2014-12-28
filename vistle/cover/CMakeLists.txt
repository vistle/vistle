cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(COVER)

add_subdirectory(Vistle)
add_subdirectory(CompositorIceT)
add_subdirectory(VncClient)
add_subdirectory(VncServer)
