add_library(vistle_module SHARED
   module.cpp
   renderer.cpp
   )

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
add_subdirectory(ShowUSG)
add_subdirectory(WriteVistle)
