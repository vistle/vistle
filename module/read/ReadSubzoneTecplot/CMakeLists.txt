<<<<<<< HEAD
set(TECIO_SHARED FALSE)
set(TECIO_MPI FALSE)
include(tecio.cmake)

set(HEADERS ReadSubzoneTecplot.h mesh.h sztecplotfile.h sources.h topo.h)

set(SOURCES ReadSubzoneTecplot.cpp mesh.cpp sztecplotfile.cpp)
#add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)
add_module(ReadSubzoneTecplot "read Sz Tecplot data " ${HEADERS} ${SOURCES})

target_link_libraries(ReadSubzoneTecplot vistle_tecio)
=======
set(HEADERS ReadIagTecplot.h mesh.h tecplotfile.h sources.h topo.h)

set(SOURCES ReadIagTecplot.cpp mesh.cpp tecplotfile.cpp)

#add_definitions(-DMARENCO)
#add_definitions(-DAHD)
add_definitions(-DDISABLE_UNUSED)
add_module(ReadIagTecplot "read IAG Tecplot data (hexahedra only)" ${HEADERS} ${SOURCES})
>>>>>>> a4e5dc83 (New reader for subzone Tecplot (.szplt))
