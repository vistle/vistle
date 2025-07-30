set(HEADERS ReadSubzoneTecplot.h mesh.h sztecplotfile.h sources.h topo.h)

set(SOURCES ReadSubzoneTecplot.cpp mesh.cpp sztecplotfile.cpp)
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/tecio/teciosrc")
#targetinclude_directories(ReadSubzoneTecplot  "${PROJECT_SOURCE_DIR}/lib/3rdparty/tecio/teciosrc")
add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)
add_module(ReadSubzoneTecplot "read Sz Tecplot data " ${HEADERS} ${SOURCES})

target_link_libraries(ReadSubzoneTecplot tecio)
