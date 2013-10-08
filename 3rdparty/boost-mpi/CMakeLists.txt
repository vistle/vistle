find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

find_package(Boost 1.40 REQUIRED COMPONENTS serialization system)

add_definitions(-DBOOST_MPI_SOURCE)
add_definitions(-DBOOST_MPI_DECL=__attribute__\ \(\(visibility\(\"default\"\)\)\))


set(boost_mpi_SOURCES
   src/broadcast.cpp
   src/communicator.cpp
   src/computation_tree.cpp
   src/content_oarchive.cpp
   src/environment.cpp
   src/exception.cpp
   src/graph_communicator.cpp
   src/group.cpp
   src/intercommunicator.cpp
   src/mpi_datatype_cache.cpp
   src/mpi_datatype_oarchive.cpp
   src/packed_iarchive.cpp
   src/packed_oarchive.cpp
   src/packed_skeleton_iarchive.cpp
   src/packed_skeleton_oarchive.cpp
   src/point_to_point.cpp
   src/request.cpp
   src/text_skeleton_oarchive.cpp
   src/timer.cpp
)

set(boost_mpi_HEADERS
)

add_library(vistle_boost_mpi ${VISTLE_LIB_TYPE} ${boost_mpi_SOURCES} ${boost_mpi_HEADERS})

target_link_libraries(vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        "include"
)
