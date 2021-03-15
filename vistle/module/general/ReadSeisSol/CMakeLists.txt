# find_package(LibXml2)
# if (LibXml2_FOUND)
#     include_directories(SYSTEM ${LIBXML2_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIRS})
#     set(EXTRA_LIBS ${LIBXML2_LIBRARIES})
# elseif()
#     message("Xdmf libs need LibXml2 to work.")
#     return()
# endif()

# find_package(TIFF)
# if (TIFF_FOUND)
#     include_directories(SYSTEM ${TIFF_INCLUDE_DIRS})
#     set(EXTRA_LIBS ${TIFF_LIBRARIES})
# elseif()
#     message("Xdmf libs need TIFF to work.")
#     return()
# endif()


# vistle_find_package(HDF5)
# # vistle_find_package(XDMF)
# if (NOT HDF5_FOUND)
#     message("Skipping ReadSeisSol because of missing HDF5")
#     return()
# else()
#     include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})
#     set(EXTRA_LIBS ${EXTRA_LIBS} ${HDF5_C_LIBRARIES} ${HDF5_CXX_LIBRARY})
# endif()

# if (NOT XDMF_FOUND)
#     message("Skipping ReadSeisSol because of missing Xdmf")
#     return()
# else()
#     include_directories(SYSTEM ${XDMF_INCLUDES})
#     set(EXTRA_LIBS ${EXTRA_LIBS} ${XDMF_C++_LIBRARY} ${XDMF_C++_CORE_LIBRARY})
# endif()

# vistle_find_package(Xdmf)
# if (NOT Xdmf_FOUND)
#     message("Skipping ReadSeisSol because of missing Xdmf")
#     return()
# elseif()
#     include_directories(SYSTEM ${XDMF_INCLUDE_DIRS})
#     set(EXTRA_LIBS ${EXTRA_LIBS} ${XDMF})
# endif()

# vistle_find_package(VTK COMPONENTS vtkCommonCore vtkIOXdmf3 vtkIOXdmf2)
vistle_find_package(VTK COMPONENTS vtkIOXdmf3)
if (NOT VTK_FOUND)
    message("Skipping ReadSeisSol because of missing VTK")
    return()
else()
    # set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})
    include_directories(SYSTEM ${VTK_INCLUDE_DIRS})
    set(EXTRA_LIBS ${EXTRA_LIBS} ${VTK_LIBRARIES} vistle_vtk)
endif()

set(HEADERS
    ReadSeisSol.h
    surfacetools.h
)

set(SOURCES
    ReadSeisSol.cpp
    surfacetools.cpp
)

add_module(ReadSeisSol ${SOURCES} ${HEADERS})

use_openmp()

target_link_libraries(ReadSeisSol ${EXTRA_LIBS})
