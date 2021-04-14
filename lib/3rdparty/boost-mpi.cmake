#vistle_find_package(Boost 1.53 REQUIRED COMPONENTS serialization system)

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
    ${BOOST_MPI_DIR}/src/timer.cpp)

if(BOOST_MPI_DIR STREQUAL "boost-mpi")
    set(boost_mpi_SOURCES ${boost_mpi_SOURCES} ${BOOST_MPI_DIR}/src/offsets.cpp ${BOOST_MPI_DIR}/src/status.cpp)
endif()

if(BOOST_MPI_DIR STREQUAL "boost-mpi-1.70")
    set(boost_mpi_SOURCES ${boost_mpi_SOURCES} ${BOOST_MPI_DIR}/src/offsets.cpp)
endif()
if (BOOST_MPI_DIR STREQUAL "boost-mpi-1.74")
   set(boost_mpi_SOURCES ${boost_mpi_SOURCES}
      ${BOOST_MPI_DIR}/src/offsets.cpp
      ${BOOST_MPI_DIR}/src/status.cpp
   )
endif()


if(BOOST_MPI_DIR STREQUAL "boost-mpi-1.69")
    set(boost_mpi_SOURCES ${boost_mpi_SOURCES} ${BOOST_MPI_DIR}/src/offsets.cpp)
endif()

if(NOT BOOST_MPI_DIR STREQUAL "boost-mpi-1.58")
    set(boost_mpi_SOURCES ${boost_mpi_SOURCES} ${BOOST_MPI_DIR}/src/error_string.cpp)
endif()

set(boost_mpi_HEADERS)

vistle_add_library(${BOOST_MPI} ${VISTLE_LIB_TYPE} ${boost_mpi_SOURCES} ${boost_mpi_HEADERS})
vistle_export_library(${BOOST_MPI} ${VISTLE_LIB_TYPE} ${boost_mpi_SOURCES} ${boost_mpi_HEADERS})

target_compile_definitions(${BOOST_MPI} PUBLIC BOOST_MPI_SOURCE)
if(NOT WIN32)
    target_compile_definitions(${BOOST_MPI} PUBLIC BOOST_MPI_DECL=__attribute__\ \(\(visibility\(\"default\"\)\)\))
endif(NOT WIN32)

target_link_libraries(
    ${BOOST_MPI} #don't use vistle_target_link_libraries here because adding dependency for boost_mpi_HEADERS::serialization brings errors.
    PRIVATE Boost::system
    PUBLIC Boost::serialization
    PUBLIC MPI::MPI_C)

get_target_property(BOOST_SERIALIZATION_INCLUDES Boost::serialization INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(${BOOST_MPI} SYSTEM INTERFACE ${BOOST_SERIALIZATION_INCLUDES})

target_include_directories(${BOOST_MPI} SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/${BOOST_MPI_DIR}/include>)
if(VISTLE_INSTALL_3RDPARTY)
    install(
        DIRECTORY ${BOOST_MPI_DIR}/include/boost
        DESTINATION include/vistle/3rdparty
        COMPONENT Devel)
    target_include_directories(${BOOST_MPI} SYSTEM PUBLIC $<INSTALL_INTERFACE:include/vistle/3rdparty>)
endif()
