
set(TECIO_SHARED FALSE)
set(TECIO_MPI FALSE)
include(tecio.cmake)

set(HEADERS ReadSubzoneTecplot.h)

set(SOURCES ReadSubzoneTecplot.cpp)
#add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)
add_module(ReadSubzoneTecplot "read Sz Tecplot data " ${HEADERS} ${SOURCES})

target_link_libraries(ReadSubzoneTecplot vistle_tecio)
