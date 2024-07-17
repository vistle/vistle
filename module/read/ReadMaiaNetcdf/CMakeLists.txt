find_package(MAIA)
if(NOT MAIA_FOUND)
    return()
endif()

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*")
ADD_DEFINITIONS(-DPVPLUGIN -DPMODEC)
set(MAIA_SOURCES
  src/timers.cpp
  ${MAIA_INCLUDE_DIR}/src/GRID/cartesiangrid.cpp
  ${MAIA_INCLUDE_DIR}/src/GRID/cartesiangridproxy.cpp
  ${MAIA_INCLUDE_DIR}/src/UTIL/debug.cpp
  ${MAIA_INCLUDE_DIR}/src/UTIL/functions.cpp
  ${MAIA_INCLUDE_DIR}/src/IO/parallelio_pnetcdf.cpp
  ${MAIA_INCLUDE_DIR}/src/UTIL/timer.cpp
  ${MAIA_INCLUDE_DIR}/src/DG/dgcartesianinterpolation.cpp
  ${MAIA_INCLUDE_DIR}/src/enums.cpp
  ${MAIA_INCLUDE_DIR}/src/COMM/globalmpiinfo.cpp
  ${MAIA_INCLUDE_DIR}/src/GRID/cartesiangrid_inst_2d.cpp
  ${MAIA_INCLUDE_DIR}/src/GRID/cartesiangrid_inst_3d.cpp
  ${MAIA_INCLUDE_DIR}/src/IO/infoout.cpp
  ${MAIA_INCLUDE_DIR}/src/MEMORY/genericobject.cpp
  ${MAIA_INCLUDE_DIR}/src/globalvariables.cpp
  ${MAIA_INCLUDE_DIR}/src/COMM/globalmpiinfo.cpp
  ${MAIA_INCLUDE_DIR}/src/MEMORY/alloc.cpp
  ${MAIA_INCLUDE_DIR}/src/COMM/mpioverride.cpp
  ${MAIA_INCLUDE_DIR}/src/IO/context.cpp

  ${MAIA_INCLUDE_DIR}/src/IO/ionetcdf.cpp
  ${MAIA_INCLUDE_DIR}/src/IO/iotoml.cpp
  ${MAIA_INCLUDE_DIR}/src/property.cpp

  #${MAIA_INCLUDE_DIR}/src/GEOM/geometry.cpp
  #${MAIA_INCLUDE_DIR}/src/GEOM/geometryionetcdf.cpp
  #${MAIA_INCLUDE_DIR}/src/GEOM/geometryproperty.cpp
  #${MAIA_INCLUDE_DIR}/src/GEOM/geometryadt.cpp
  #${MAIA_INCLUDE_DIR}/src/GEOM/geometrycontext.cpp
)

if(STRCTRD)
  message("Appending src/strctrd.cpp to server_sources")
  list(APPEND MAIA_SOURCES
    ${MAIA_INCLUDE_DIR}/src/IO/parallelio_hdf5.cpp
    src/strctrd.cpp
  )
endif()

add_module(ReadMaiaNetcdf "read netCdf data form RWTH mAia CFD code " ReadMaiaNetcdf.cpp ReadMaiaNetcdf.h readerbase.h ${MAIA_SOURCES})
target_include_directories(ReadMaiaNetcdf PRIVATE
  src
  ${MAIA_INCLUDE_DIRS}
  ${MAIA_INCLUDE_DIRS}/src
  ${MAIA_INCLUDE_DIRS}/include
)

target_compile_definitions(ReadMaiaNetcdf PRIVATE PARALLELIO_DEFAULT_BACKEND=ParallelIoPNetcdf)
message(MAIA_INCLUDE_DIRS ${MAIA_INCLUDE_DIRS})
vistle_find_package(HDF5 COMPONENTS C HL REQUIRED)
vistle_find_package(PNETCDF REQUIRED)
vistle_find_package(NETCDF REQUIRED)
if(PNETCDF_FOUND)
    target_link_libraries(ReadMaiaNetcdf ${PNETCDF_LIBRARIES})
    target_include_directories(ReadMaiaNetcdf SYSTEM PRIVATE ${PNETCDF_INCLUDE_DIRS})
    message("linking ReadMaiaNetcdf against pnetcdf")
endif()
# vistle_find_package(NETCDF)
# if(NETCDF_FOUND)
#     add_module(ReadMaiaNetcdf "Read Duisburg files" ${SOURCES} ${HEADERS})
#     target_compile_definitions(ReadMaiaNetcdf PRIVATE USE_NETCDF)
#     if(NETCDF_PARALLEL)
#         target_compile_definitions(ReadMaiaNetcdf PRIVATE NETCDF_PARALLEL)
#     endif()
#     target_link_libraries(ReadMaiaNetcdf ${NETCDF_LIBRARIES} vistle_netcdf)
#     target_include_directories(ReadMaiaNetcdf SYSTEM PRIVATE ${NETCDF_INCLUDE_DIRS})
# endif()

target_link_libraries(ReadMaiaNetcdf ${NETCDF_LIBRARIES} HDF5::HDF5)


