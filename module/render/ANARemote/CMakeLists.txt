vistle_find_package(anari)
if(NOT anari_FOUND)
    return()
endif()

add_module(
    ANARemote
    "ANARI remote renderer"
    anaremote.cpp
    projection.h
    projection.cpp
    anarirenderobject.h
    anarirenderobject.cpp)

target_link_libraries(ANARemote anari::anari vistle_renderer vistle_rhr)
