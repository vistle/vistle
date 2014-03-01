include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS}")

SET(EXTRA_LIBS ${EXTRA_LIBS} ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
ADD_DEFINITIONS(-DHAS_MPI)
ADD_DEFINITIONS(-DRENAME_MAIN=realmain)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PYTHON_INCLUDE_PATH}
        ..
)

add_executable(COVER coverwrapper.cpp)
target_link_libraries(COVER ${EXTRA_LIBS} vistle_util)
if(NOT APPLE AND NOT WINDOWS)
   target_link_libraries(COVER ${EXTRA_LIBS} -ldl)
endif()
