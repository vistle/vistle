set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
vistle_find_package(COVER)
if (NOT COVER_FOUND)
   message("COVER not found - not building COVER wrapper")
   return()
endif()

include_directories(${MPI_C_INCLUDE_PATH})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

SET(EXTRA_LIBS ${EXTRA_LIBS} ${MPI_C_LIBRARIES})
ADD_DEFINITIONS(-DHAS_MPI)
ADD_DEFINITIONS(-DRENAME_MAIN=realmain)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

add_module(COVER coverwrapper.cpp)
target_link_libraries(COVER ${EXTRA_LIBS} vistle_util)
if(NOT APPLE AND NOT WINDOWS)
   target_link_libraries(COVER ${CMAKE_DL_LIBS})
endif()
