find_package(MPI REQUIRED)
include_directories(${MPI_C_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS})

find_package(Boost 1.40 REQUIRED COMPONENTS serialization system)

add_definitions(-DBOOST_MPI_SOURCE)
add_definitions(-DBOOST_MPI_DECL=__attribute__\ \(\(visibility\(\"default\"\)\)\))


set(boost_mpi_SOURCES
   ${BOOST_MPI_DIR}/src/broadcast.cpp
   ${BOOST_MPI_DIR}/src/communicator.cpp
   ${BOOST_MPI_DIR}/src/computation_tree.cpp
   ${BOOST_MPI_DIR}/src/content_oarchive.cpp
   ${BOOST_MPI_DIR}/src/environment.cpp
   ${BOOST_MPI_DIR}/src/exception.cpp
   ${BOOST_MPI_DIR}/src/graph_communicator.cpp
   ${BOOST_MPI_DIR}/src/group.cpp
   ${BOOST_MPI_DIR}/src/intercommunicator.cpp
   ${BOOST_MPI_DIR}/src/mpi_datatype_cache.cpp
   ${BOOST_MPI_DIR}/src/mpi_datatype_oarchive.cpp
   ${BOOST_MPI_DIR}/src/packed_iarchive.cpp
   ${BOOST_MPI_DIR}/src/packed_oarchive.cpp
   ${BOOST_MPI_DIR}/src/packed_skeleton_iarchive.cpp
   ${BOOST_MPI_DIR}/src/packed_skeleton_oarchive.cpp
   ${BOOST_MPI_DIR}/src/point_to_point.cpp
   ${BOOST_MPI_DIR}/src/request.cpp
   ${BOOST_MPI_DIR}/src/text_skeleton_oarchive.cpp
   ${BOOST_MPI_DIR}/src/timer.cpp
)

set(boost_mpi_HEADERS
)

add_library(vistle_boost_mpi ${VISTLE_LIB_TYPE} ${boost_mpi_SOURCES} ${boost_mpi_HEADERS})

target_link_libraries(vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)

include_directories(
        "${BOOST_MPI_DIR}/include"
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
