set(HEADERS
   ReadIagTecplot.h
   mesh.h
   tecplotfile.h
   sources.h
   topo.h
)

set(SOURCES
   ReadIagTecplot.cpp
   mesh.cpp
   tecplotfile.cpp
)

add_module(ReadIagTecplot ${HEADERS} ${SOURCES})

include_directories(
   ${Boost_INCLUDE_DIRS}
   ${MPI_C_INCLUDE_PATH}
   ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ReadIagTecplot
   ${MPI_C_LIBRARIES}
   ${VTK_LIBRARIES}
   vistle_module
)

