set(module_SOURCES
   module.cpp
)

set(module_HEADERS
   export.h
   module.h
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
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)

include_directories(
        ../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

macro(add_module targetname)

    add_executable(${targetname} ${ARGN})

    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libexec/module)
endmacro(add_module)


add_subdirectory(Add)
add_subdirectory(AttachGrid)
add_subdirectory(AttachNormals)
add_subdirectory(AttachObject)
add_subdirectory(AttachShader)
add_subdirectory(Cache)
add_subdirectory(CellToVert)
add_subdirectory(Collect)
add_subdirectory(Color)
add_subdirectory(ColorAttribute)
add_subdirectory(CreateCelltree)
add_subdirectory(CutGeometry)
add_subdirectory(CuttingSurface)
add_subdirectory(CuttingSurfaceOld)
add_subdirectory(Extrema)
add_subdirectory(Gendat)
add_subdirectory(GenIsoDat)
add_subdirectory(IsoSurface)
add_subdirectory(IsoSurfaceOld)
add_subdirectory(MetaData)
add_subdirectory(MpiInfo)
add_subdirectory(ObjectStatistics)
add_subdirectory(PrintAttributes)
add_subdirectory(ReadCovise)
add_subdirectory(ReadFoam)
add_subdirectory(ReadVistle)
add_subdirectory(Replicate)
add_subdirectory(ShowUSG)
add_subdirectory(ShowCelltree)
add_subdirectory(Spheres)
add_subdirectory(Tubes)
add_subdirectory(TestCellSearch)
add_subdirectory(TestInterpolation)
add_subdirectory(ToPoints)
add_subdirectory(ToTriangles)
add_subdirectory(WriteVistle)
add_subdirectory(Tracer)
add_subdirectory(VecToScalar)
add_subdirectory(CreateVertexOwnerList)

