if(NOT TARGET anari::anari)
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

target_link_libraries(ANARemote PRIVATE vistle::viskores_anari anari::anari vistle_renderer vistle_rhr)
