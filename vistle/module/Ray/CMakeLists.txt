#find_package(IceT REQUIRED)
set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/3rdparty/IceT/src/include)
set(ICET_CORE_LIBS "IceTCore")
set(ICET_MPI_LIBS "IceTMPI")

find_package(LibVncServer)
find_package(embree)                                                                                                     
find_package(TBB)
set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(COVISE OPTIONAL_COMPONENTS RHR)
using(SNAPPY optional)
find_package(ZLIB)
find_package(JPEG)

if(NOT COVISE_RHR_FOUND)
   message("Ray: COVISE RHR not found")
   return()
endif()

if(NOT LIBVNCSERVER_FOUND)
   message("Ray: libvncserver not found")
   return()
endif()

if(NOT SNAPPY_FOUND)
   message("Ray: Snappy not found")
   return()
endif()

if(NOT EMBREE_FOUND)
   message("Ray: Embree not found")
   return()
endif()

use_openmp()

if(TBB_FOUND)
    #if (NOT OPENMP_FOUND)
      add_definitions(-DUSE_TBB)
      #endif()
endif()

add_module(Ray
   renderobject.cpp
   ray.cpp
   vncserver.cpp
   )

include_directories(
        ${Boost_INCLUDE_DIRS}
        ../../../3rdparty/boost-mpi/include
        ${MPI_C_INCLUDE_PATH}
        ${ICET_INCLUDE_DIRS}
        ${LIBVNCSERVER_INCLUDE_DIR}
        ${COVISE_RHR_INCLUDE_DIRS}
        ${SNAPPY_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}
        ${EMBREE_INCLUDE_PATH}
        ${TBB_INCLUDE_DIRS}
)

target_link_libraries(Ray
        ${Boost_LIBRARIES}
        vistle_boost_mpi
        ${MPI_C_LIBRARIES}
        vistle_module
        ${ICET_CORE_LIBS}
        ${ICET_MPI_LIBS}
        ${LIBVNCSERVER_SERVER_LIBRARY}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${COVISE_RHR_LIBRARIES}
        ${SNAPPY_LIBRARIES}
        ${EMBREE_LIBRARIES}
        ${TBB_LIBRARIES}
)
