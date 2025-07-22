set(HEADERS ReadSubzoneTecplot.h mesh.h sztecplotfile.h sources.h topo.h)

set(SOURCES ReadSubzoneTecplot.cpp mesh.cpp sztecplotfile.cpp)
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/vistle/lib/3rdparty/tecio/teciosrc/")

#add_definitions(-DMARENCO)
#add_definitions(-DAHD)
#add_definitions(-DDISABLE_UNUSED)
add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)
add_module(ReadSubzoneTecplot "read Sz Tecplot data " ${HEADERS} ${SOURCES})
