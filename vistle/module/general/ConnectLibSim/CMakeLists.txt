# Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
# Project developers.  See the top-level LICENSE file for dates and other
# details.  No copyright assignment is required to contribute to VisIt.

#****************************************************************************
# Modifications:
#
#****************************************************************************


option(LIBSIM_DEBUG "print debuf messages" OFF)
if(LIBSIM_DEBUG)
   add_definitions(-DLIBSIM_DEBUG)
endif()
SET(RUNTIME_SOURCES 
  EngineMessage.cpp
  Exeption.cpp
  ConnectLibSim.cpp
  VisItControlInterfaceRuntime.C 
  VisItDataInterfaceRuntime.C 
  VisItDataInterfaceRuntimeP.C 
  CommandMetaData.C
  CurveData.C
  CurveMetaData.C
  CurvilinearMesh.C
  CSGMesh.C
  #DomainBoundaries.C
  DomainList.C 
  #DomainNesting.C
  Engine.cpp
  ExpressionMetaData.C
  MaterialData.C
  MaterialMetaData.C
  MeshMetaData.C
  MessageMetaData.C
  ModuleInterface.cpp
  NameList.C
  OptionList.C
  PointMesh.C
  ProxyModule.cpp
  RectilinearMesh.C
  SimulationMetaData.C
  SpeciesData.C
  SpeciesMetaData.C
  UnstructuredMesh.C
  VariableData.C
  VariableMetaData.C
  #View2D.C
  #View3D.C
)

SET(RUNTIME_HEADERS 
  EngineMessage.h
  Exeption.h
  ConnectLibSim.h
  VisItControlInterfaceRuntime.h
  VisItDataInterfaceRuntime.h
  VisItDataInterfaceRuntimeP.h
  VisItDataTypes.h 
  CommandMetaData.h
  CurveData.h
  CurveMetaData.h
  CurvilinearMesh.h
  CSGMesh.h
  #DomainBoundaries.h
  DomainList.h 
  #DomainNesting.h
  Engine.h
  ExpressionMetaData.h
  MaterialData.h
  MaterialMetaData.h
  MeshMetaData.h
  MessageMetaData.h
  MetaData.h
  ModuleInterface.h
  NameList.h
  OptionList.h
  PointMesh.h
  ProxyModule.h
  RectilinearMesh.h
  SimulationMetaData.h
  SpeciesData.h
  SpeciesMetaData.h
  TransformArray.h
  UnstructuredMesh.h
  VariableData.h
  VariableMetaData.h
  VectorTypes.h
  #View2D.h
  #View3D.h
  VisItExports.h
)

set (LIBCORENAME simV2runtime_par)
set(LIBNAME lib${LIBCORENAME}) #VisItExports has to be adopted if this changes
if(NOT MSVC)
	set(LIBNAME ${LIBCORENAME}) #to compensate the additional lib prefix from vistle_add_library
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

vistle_add_library(${LIBNAME} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES})

target_link_libraries(${LIBNAME}
		vistle_module
		vistle_util
		vistle_control
        vistle_core
        vistle_boost_mpi
		libsim_connect
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)
if(NOT VISTLE_MULTI_PROCESS)
target_link_libraries(${LIBNAME}
		vistle_clustermanager)
endif()

#LibSimControlModule
add_module(LibSimControlModule LibSimControlModule.h LibSimControlModule.cpp)
target_link_libraries(LibSimControlModule
	vistle_module
	vistle_util
	libsim_connect
	${LIBNAME}
	${Boost_LIBRARIES}
	)
	
	
add_module(ConnectLibSim ConnectLibSimExec.cpp) 
target_link_libraries(ConnectLibSim
	${LIBNAME}
)


