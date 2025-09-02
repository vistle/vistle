if(NOT VISTLE_USE_MPI)
    return()
endif()

using(OpenGL)
using(Boost)

include_directories(SYSTEM ${ICET_INCLUDE_DIRS})

set(HEADERS CompositorIceT.h)
set(SOURCES CompositorIceT.cpp)

cover_add_plugin(CompositorIceT ${HEADER} ${SOURCES})
target_link_libraries(CompositorIceT vistle_rhr VistlePluginUtil ${ICET_CORE_LIBS} ${ICET_MPI_LIBS} MPI::MPI_CXX)
