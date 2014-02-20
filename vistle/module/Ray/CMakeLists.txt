#find_package(IceT REQUIRED)
set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/3rdparty/IceT/src/include)
set(ICET_CORE_LIBS "IceTCore")
set(ICET_MPI_LIBS "IceTMPI")

find_package(LibVncServer)
if(NOT LIBVNCSERVER_FOUND)
   return()
endif()

set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(COVER)
if(NOT COVER_FOUND)
   message("Ray: COVER not found")
   return()
endif()

using(SNAPPY optional)
if(NOT SNAPPY_FOUND)
   message("Ray: Snappy not found")
   return()
endif()

find_package(embree)                                                                                                     
if(NOT EMBREE_FOUND)
   message("Ray: Embree not found")
   return()
endif()

find_package(TBB)
if(TBB_FOUND)
   #add_definitions(-DUSE_TBB)
endif()

#use_openmp()

add_executable(Ray
   renderobject.cpp
   ray.cpp
   framebuffer.cpp
   vncserver.cpp
   )

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${ICET_INCLUDE_DIRS}
        ${LIBVNCSERVER_INCLUDE_DIR}
        ${COVER_RHR_INCLUDE_DIR}
        ${SNAPPY_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}
        ${EMBREE_INCLUDE_PATH}
        ${TBB_INCLUDE_DIRS}
)

target_link_libraries(Ray
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
        ${ICET_CORE_LIBS}
        ${ICET_MPI_LIBS}
        ${LIBVNCSERVER_SERVER_LIBRARY}
        ${COVER_RHR_LIBRARY}
        ${SNAPPY_LIBRARIES}
        ${EMBREE_LIBRARY}
        ${TBB_LIBRARIES}
)
