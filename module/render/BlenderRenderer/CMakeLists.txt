add_module(BlenderRenderer "Blender remote renderer" BlenderRenderer.cpp)

target_link_libraries(BlenderRenderer vistle_renderer vistle_util)

if(NOT VISTLE_MULTI_PROCESS)
    if(APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${VISTLE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
    elseif(WIN32)

    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${VISTLE_SHARED_LINKER_FLAGS} -Wl,--allow-shlib-undefined")
    endif()
endif()
