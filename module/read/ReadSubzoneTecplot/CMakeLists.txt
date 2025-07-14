set(HEADERS ReadSubzoneTecplot.h mesh.h sztecplotfile.h sources.h topo.h)

set(SOURCES ReadSubzoneTecplot.cpp mesh.cpp sztecplotfile.cpp)

#add_definitions(-DMARENCO)
#add_definitions(-DAHD)
#add_definitions(-DDISABLE_UNUSED)
add_module(ReadSubzoneTecplot "read Sz Tecplot data " ${HEADERS} ${SOURCES})
