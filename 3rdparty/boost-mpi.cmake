find_package(MPI REQUIRED)
include_directories(${MPI_C_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS})

find_package(Boost 1.40 REQUIRED COMPONENTS serialization system)

add_definitions(-DBOOST_MPI_SOURCE)
add_definitions(-DBOOST_MPI_DECL=__attribute__\ \(\(visibility\(\"default\"\)\)\))


set(boost_mpi_SOURCES
   boost-mpi/src/broadcast.cpp
   boost-mpi/src/communicator.cpp
   boost-mpi/src/computation_tree.cpp
   boost-mpi/src/content_oarchive.cpp
   boost-mpi/src/environment.cpp
   boost-mpi/src/exception.cpp
   boost-mpi/src/graph_communicator.cpp
   boost-mpi/src/group.cpp
   boost-mpi/src/intercommunicator.cpp
   boost-mpi/src/mpi_datatype_cache.cpp
   boost-mpi/src/mpi_datatype_oarchive.cpp
   boost-mpi/src/packed_iarchive.cpp
   boost-mpi/src/packed_oarchive.cpp
   boost-mpi/src/packed_skeleton_iarchive.cpp
   boost-mpi/src/packed_skeleton_oarchive.cpp
   boost-mpi/src/point_to_point.cpp
   boost-mpi/src/request.cpp
   boost-mpi/src/text_skeleton_oarchive.cpp
   boost-mpi/src/timer.cpp
)

set(boost_mpi_HEADERS
)

add_library(vistle_boost_mpi ${VISTLE_LIB_TYPE} ${boost_mpi_SOURCES} ${boost_mpi_HEADERS})

target_link_libraries(vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)

include_directories(
        "boost-mpi/include"
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
