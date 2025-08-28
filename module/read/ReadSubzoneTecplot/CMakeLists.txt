
set(TECIO_SHARED FALSE)
set(TECIO_MPI TRUE)
include(tecio.cmake)

set(HEADERS ReadSubzoneTecplot.h)

set(SOURCES ReadSubzoneTecplot.cpp)

#add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)
add_module(ReadSubzoneTecplot "read Sz Tecplot data " ${HEADERS} ${SOURCES})

target_link_libraries(ReadSubzoneTecplot vistle_tecio)

# If tecio.cmake does NOT already do this, add:
if(TECIO_MPI)
  find_package(MPI REQUIRED)
  target_compile_definitions(ReadSubzoneTecplot PRIVATE TECIOMPI)
  target_link_libraries(ReadSubzoneTecplot MPI::MPI_CXX)
endif()
