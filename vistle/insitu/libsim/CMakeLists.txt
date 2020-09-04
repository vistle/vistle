option(LIBSIM_DEBUG "print debug messages" OFF)
option(LIBSIM_PARALLEL "build parallel instead of serial LibSim Interface" ON)

add_subdirectory(engineInterface)
add_subdirectory(connectLibsim)
add_subdirectory(libsimInterface)



SET(RUNTIME_SOURCES 
  DataProcessor.cpp
  Engine.cpp
  Exeption.cpp
  RectilinearMesh.cpp
  StructuredMesh.cpp
  UnstructuredMesh.cpp
  VisItControlInterfaceRuntime.C
)

SET(RUNTIME_HEADERS 
  MeshInfo.h
  StructuredMesh.h  
  Engine.h
  DataProcessor.h
  Exeption.h
  export.h
  MetaData.h
  RectilinearMesh.h
  UnstructuredMesh.h
  VertexTypesToVistle.h  
  VisItControlInterfaceRuntime.h
  VisitDataTypesToVistle.h  
)

set (LIBCORENAME simV2runtime_par) # use disable LIBSIM_PARALLEL CMake option for serial simulation
if(NOT LIBSIM_PARALLEL)
set (LIBCORENAME simV2runtime_ser) 
endif()
set(LIBNAME lib${LIBCORENAME}) #export.h has to be adopted if this changes
if(NOT MSVC)
	set(LIBNAME ${LIBCORENAME}) #to compensate the additional lib prefix from vistle_add_library
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

vistle_add_library(${LIBNAME} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES})
vistle_export_library(${LIBNAME} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES})


if(LIBSIM_DEBUG)
   target_compile_definitions(${LIBNAME} PRIVATE -DLIBSIM_DEBUG)
endif()
if (VISTLE_SHM_PER_RANK)
   target_compile_definitions(${LIBNAME} PRIVATE -DSHMPERRANK)
endif()
target_compile_definitions(${LIBNAME} PRIVATE -DLIBSIM_EXPORT)

target_compile_features(${LIBNAME} PRIVATE cxx_std_14)

vistle_target_link_libraries(${LIBNAME}
		PRIVATE Boost::boost
        PRIVATE MPI::MPI_C
        PRIVATE Threads::Threads
        PRIVATE vistle_boost_mpi
        PRIVATE vistle_core
		PRIVATE vistle_insitu_core
		PRIVATE vistle_insitu_message
		PRIVATE vistle_libsim
		PRIVATE vistle_module
		PRIVATE vistle_util
		)
		
if(NOT VISTLE_MULTI_PROCESS)
target_link_libraries(${LIBNAME}
		PRIVATE vistle_clustermanager
		PRIVATE vistle_libsim_engine_interface
		)
endif()

vistle_install_docs(README.md)

