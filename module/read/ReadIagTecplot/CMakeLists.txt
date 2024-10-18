set(HEADERS ReadIagTecplot.h mesh.h tecplotfile.h sources.h topo.h)

set(SOURCES ReadIagTecplot.cpp mesh.cpp tecplotfile.cpp)

#add_definitions(-DMARENCO)
#add_definitions(-DAHD)
add_definitions(-DDISABLE_UNUSED)
add_module(ReadIagTecplot "read IAG Tecplot data (hexahedra only)" ${HEADERS} ${SOURCES})
