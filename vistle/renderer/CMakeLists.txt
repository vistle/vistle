set(renderer_SOURCES renderer.cpp renderobject.cpp)

set(renderer_HEADERS export.h renderer.h renderobject.h)

use_openmp()
if(TBB_FOUND)
    set(renderer_SOURCES ${renderer_SOURCES} rhrcontroller.cpp parrendmgr.cpp)

    set(renderer_HEADERS ${renderer_HEADERS} parrendmgr.h rhrcontroller.h)
endif()

vistle_add_library(vistle_renderer ${VISTLE_LIB_TYPE} ${renderer_SOURCES} ${renderer_HEADERS})
vistle_install_headers(renderer ${renderer_HEADERS})

if(TBB_FOUND)
    target_include_directories(
        vistle_renderer SYSTEM
        PUBLIC ${TBB_INCLUDE_DIR}
        PUBLIC ${ICET_INCLUDE_DIRS})

    target_link_libraries(
        vistle_renderer
        PUBLIC vistle_rhr
        PUBLIC ${ICET_CORE_LIBS}
        PUBLIC ${ICET_MPI_LIBS})
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(vistle_renderer PUBLIC -lrt)
endif()

target_link_libraries(
    vistle_renderer
    PUBLIC vistle_util
    PUBLIC vistle_core
    PUBLIC vistle_module
    PUBLIC ${BOOST_MPI}
    PUBLIC Boost::boost
    PUBLIC MPI::MPI_C)

add_subdirectory(COVER)
add_subdirectory(OsgRenderer)
add_subdirectory(DisCOVERay)
