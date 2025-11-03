set(TECIO_SHARED TRUE)
set(TECIO_MPI FALSE)
include(tecio.cmake)

set(HEADERS ReadSubzoneTecplot.h)

set(SOURCES ReadSubzoneTecplot.cpp)

#add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)
add_module(ReadSubzoneTecplot "read SZL (Sub-Zone Load-on-demand) Tecplot data" ${HEADERS} ${SOURCES})

target_link_libraries(ReadSubzoneTecplot PRIVATE vistle_tecio)
