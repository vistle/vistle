set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libexec/module)

set(module_SOURCES
   module.cpp
   renderer.cpp
)

set(module_HEADERS
   export.h
   module.h
   renderer.h
)

add_library(vistle_module ${VISTLE_LIB_TYPE} ${module_SOURCES} ${module_HEADERS})

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_module
		-lrt
	)
endif()

target_link_libraries(vistle_module
        vistle_util
        vistle_core
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ..
)

add_subdirectory(Add)
add_subdirectory(CellToVert)
add_subdirectory(Collect)
add_subdirectory(Color)
add_subdirectory(CutGeometry)
add_subdirectory(CuttingSurface)
add_subdirectory(Extrema)
add_subdirectory(Gendat)
add_subdirectory(IsoSurface)
add_subdirectory(OSGRenderer)
add_subdirectory(ReadCovise)
add_subdirectory(ReadFOAM)
add_subdirectory(ReadVistle)
add_subdirectory(Replicate)
add_subdirectory(ShowUSG)
add_subdirectory(WriteVistle)
