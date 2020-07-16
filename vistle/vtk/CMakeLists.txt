vistle_find_package(VTK
   COMPONENTS vtkIOLegacy vtkIOXML vtkFiltersCore vtkCommonCore vtkImagingCore vtkCommonDataModel vtkCommonExecutionModel
   NO_MODULE)

if (NOT VTK_FOUND)
    return()
endif()



set(SOURCES 
	vtktovistle.cpp
)

set(HEADERS
	export.h
	vtktovistle.h
)


vistle_add_library(vistle_vtk 
	${SOURCES}
	${HEADERS}
)
vistle_export_library(vistle_vtk 
	${SOURCES}
	${HEADERS}
)


if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   target_compile_options(vistle_vtk PRIVATE -Wno-deprecated)
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
   target_compile_options(vistle_vtk PRIVATE -Wno-deprecated -Wno-inconsistent-missing-override)
endif()

target_include_directories(vistle_vtk SYSTEM
   PUBLIC ${VTK_INCLUDE_DIRS}
)

vistle_target_link_libraries(vistle_vtk
   PUBLIC ${VTK_LIBRARIES}
   PUBLIC vistle_core
)
target_compile_definitions(vistle_vtk PUBLIC ${VTK_DEFINITIONS})
