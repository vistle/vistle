option(LIBSIM_DEBUG "print debug messages" OFF)
option(LIBSIM_PARALLEL "build parallel instead of serial LibSim Interface" ON)

add_subdirectory(engineInterface)
add_subdirectory(connectLibsim)
#add_subdirectory(moduleOption)
include(libsimInterface/libsim_interface.cmake)

SET(RUNTIME_SOURCES 
  DataTransmitter.cpp
  Engine.cpp
  Exeption.cpp
  MetaData.cpp
  RectilinearMesh.cpp
  StructuredMesh.cpp
  UnstructuredMesh.cpp
  VariableData.cpp
  VertexTypesToVistle.cpp
  VisItControlInterfaceRuntime.C
)

SET(RUNTIME_HEADERS 
  ArrayStruct.h
  DataTransmitter.h
  Engine.h
  Exeption.h
  MeshInfo.h
  MetaData.h
  RectilinearMesh.h
  StructuredMesh.h  
  UnstructuredMesh.h
  VariableData.h  
  VariableInfo.h  
  VertexTypesToVistle.h  
  VisItControlInterfaceRuntime.h
  VisitDataTypesToVistle.h  
  IntOption.h
  export.h
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

vistle_add_library(${LIBNAME} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES} ${LIBSIM_INTERFACE_FILES})
vistle_export_library(${LIBNAME} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES})


if(LIBSIM_DEBUG)
   target_compile_definitions(${LIBNAME} PRIVATE -DLIBSIM_DEBUG)
endif()
target_compile_definitions(${LIBNAME} PRIVATE -DLIBSIM_EXPORT)

# By default, symbols provided by the sublibs that are not used by mainlib (which is all of them in this case)
# are not used. This changes that.
if (NOT WIN32)
add_definitions("-fPIC")
endif ()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fPIC")
endif()

target_compile_features(${LIBNAME} PRIVATE cxx_std_14)

vistle_target_link_libraries(${LIBNAME}
		PRIVATE Boost::boost
		PRIVATE vistle_insitu_core
		PRIVATE vistle_insitu_message
		PRIVATE vistle_module
		PRIVATE vistle_util
		PRIVATE vistle_util_mpi
      PRIVATE MPI::MPI_C
      PRIVATE Threads::Threads
      PRIVATE vistle_boost_mpi
      PRIVATE vistle_core
		)
		
if(NOT VISTLE_MULTI_PROCESS)
target_link_libraries(${LIBNAME}
		PRIVATE vistle_clustermanager
		PRIVATE vistle_libsim_engine_interface
		)
endif()

vistle_install_docs(README.md)

