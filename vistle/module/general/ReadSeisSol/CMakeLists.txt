# vistle_find_package(HDF5)
# if (NOT HDF5_FOUND)
#     message("Skipping ReadSeisSol because of missing HDF5")
#     return()
# else()
#     include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})
#     set(EXTRA_LIBS ${EXTRA_LIBS} ${HDF5_C_LIBRARIES} ${HDF5_CXX_LIBRARY})
# endif()

vistle_find_package(Xdmf)
if (NOT Xdmf_FOUND)
    message("Skipping ReadSeisSol because of missing Xdmf")
    return()
endif()

set(HEADERS
    ReadSeisSol.h
    surfacetools.h
)

set(SOURCES
    ReadSeisSol.cpp
    surfacetools.cpp
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_module(ReadSeisSol "read ChEESE Seismic Data files (xmdf/hdf5)" ${SOURCES} ${HEADERS})

use_openmp()

# target_link_libraries(ReadSeisSol ${} ${EXTRA_LIBS})
#target_link_libraries(ReadSeisSol ${XDMFCORE_LIBRARY} ${XDMF_LIBNAME})
target_link_libraries(ReadSeisSol XdmfCore ${XDMF_LIBNAME})
target_include_directories(ReadSeisSol PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
