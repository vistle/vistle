set(SOURCES VistleManagerPlugin.cpp)

cover_add_plugin(VistleManagerPlugin ${HEADERS} ${SOURCES})
target_link_libraries(
    VistleManagerPlugin
    VistlePluginUtil
    vistle_cover
    vistle_renderer
    vistle_control
    vistle_clustermanager
    ${BOOST_MPI}
    ${COVISE_NET_LIBRARY})

set_target_properties(VistleManagerPlugin PROPERTIES OUTPUT_NAME "VistleManager")
target_include_directories(
    VistleManagerPlugin
    PRIVATE ${COVISE_INCLUDE_DIRS}
    PRIVATE ${COVER_INCLUDE_DIRS}
    PRIVATE .. # VistlePluginUtil
)

target_compile_definitions(VistleManagerPlugin PRIVATE COVER_PLUGIN)

if(NOT VISTLE_MULTI_PROCESS)
    if(VISTLE_USE_OPENMP AND TARGET OpenMP::OpenMP_CXX)
        target_link_libraries(vistle_manager PRIVATE OpenMP::OpenMP_CXX)
    endif()
    if(NOT VISTLE_MODULES_SHARED)
        foreach(m ${ALL_MODULES})
            if(APPLE)
                target_link_libraries(VistleManagerPlugin -Wl,-force_load ${m})
            else()
                target_link_libraries(VistleManagerPlugin -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
            endif()
            #target_link_libraries(vistle_clustermanager $<TARGET_OBJECTS:${m}>)
        endforeach()
        target_link_libraries(VistleManagerPlugin vistle_module_descriptions)
    endif()
endif()
