set(renderer_SOURCES renderer.cpp renderobject.cpp)

set(renderer_HEADERS export.h renderer.h renderobject.h)

set(renderer_SOURCES ${renderer_SOURCES} rhrcontroller.cpp parrendmgr.cpp)
set(renderer_HEADERS ${renderer_HEADERS} parrendmgr.h rhrcontroller.h)

set(visibility EXPORT)
if(NOT VISTLE_BUILD_SHARED)
    set(visibility INTERNAL)
endif()
vistle_add_library(vistle_renderer ${visibility} ${VISTLE_LIB_TYPE} ${renderer_SOURCES} ${renderer_HEADERS})

target_link_libraries(vistle_renderer PUBLIC vistle_rhr)

if(UNIX AND NOT APPLE)
    target_link_libraries(vistle_renderer PUBLIC -lrt)
endif()

target_link_libraries(
    vistle_renderer
    PUBLIC vistle_util
    PUBLIC vistle_core
    PUBLIC vistle_alg
    PUBLIC vistle_module
    PUBLIC vistle_config
    PUBLIC ${BOOST_MPI}
    PUBLIC Boost::boost
    PUBLIC MPI::MPI_CXX)

if(VISTLE_USE_MPI)
    target_include_directories(vistle_renderer SYSTEM PRIVATE ${ICET_INCLUDE_DIRS})
    target_link_libraries(
        vistle_renderer
        PRIVATE ${ICET_CORE_LIBS}
        PRIVATE ${ICET_MPI_LIBS})
    target_compile_definitions(vistle_renderer PRIVATE VISTLE_USE_MPI)
endif()
