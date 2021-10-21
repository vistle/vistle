vistle_find_package(Xdmf)
if(NOT Xdmf_FOUND)
    return()
endif()

set(HEADERS ReadSeisSol.h surfacetools.h)
set(SOURCES ReadSeisSol.cpp surfacetools.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_module(ReadSeisSol "read ChEESE seismic data files (XDMF/HDF5)" ${SOURCES} ${HEADERS})

use_openmp()

target_link_libraries(ReadSeisSol vistle_xdmf XdmfCore ${XDMF_LIBNAME})
target_include_directories(ReadSeisSol PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
