if(NOT VISTLE_USE_MPI)
    return()
endif()

using(OpenGL)
using(Boost)

include_directories(SYSTEM ${ICET_INCLUDE_DIRS})
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

set(HEADERS CompositorIceT.h)
set(SOURCES CompositorIceT.cpp)

cover_add_plugin(CompositorIceT ${HEADER} ${SOURCES})
target_link_libraries(CompositorIceT vistle_rhr VistlePluginUtil ${ICET_CORE_LIBS} ${ICET_MPI_LIBS} ${MPI_CXX_LIBRARIES})
