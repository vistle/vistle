set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
vistle_find_package(COVER)
if (NOT COVER_FOUND)
   message("COVER not found - not building COVER wrapper")
   return()
endif()

include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

SET(EXTRA_LIBS ${EXTRA_LIBS} ${MPI_C_LIBRARIES})
ADD_DEFINITIONS(-DHAS_MPI)
ADD_DEFINITIONS(-DCOVER_PLUGIN)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
)
include_directories(
        ${COVER_INCLUDE_DIRS}
        ../../cover
        ..
)

vistle_add_library(vistle_cover SHARED 
   OsgRenderer.cpp OsgRenderer.h
   VistleGeometryGenerator.h VistleGeometryGenerator.cpp
)
target_link_libraries(vistle_cover ${EXTRA_LIBS} vistle_renderer vistle_util
   ${CMAKE_DL_LIBS} VistlePluginUtil ${COVER_PLUGINUTIL_LIBRARY})

add_module(COVER coverwrapper.cpp)
target_link_libraries(COVER ${EXTRA_LIBS} vistle_util ${CMAKE_DL_LIBS} vistle_cover)
if(NOT VISTLE_MULTI_PROCESS)
if(APPLE)
   set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
elseif(WINDOWS)
else()
   set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,undefs")
endif()
endif()
