# Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
# Project developers.  See the top-level LICENSE file for dates and other
# details.  No copyright assignment is required to contribute to VisIt.

#****************************************************************************
# Modifications:
#
#****************************************************************************



SET(RUNNTIME_SOURCES 
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

SET(RUNNTIME_HEADERS 
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
  ModuleInterface.h
  NameList.h
  OptionList.h
  PointMesh.h
  RectilinearMesh.h
  SimulationMetaData.h
  SpeciesData.h
  SpeciesMetaData.h
  UnstructuredMesh.h
  VariableData.h
  VariableMetaData.h
  Vectortypes.h
  #View2D.h
  #View3D.h
  VisItExports.h
)

set(MODULE_SOURCES
  ConnectLibSim.cpp
)

set(MODULE_HEADERS
  ConnectLibSim.h
)
vistle_add_library(libsimV2runntime_par ${RUNNTIME_HEADERS} ${RUNNTIME_SOURCES})

target_link_libraries(libsimV2runntime_par
        vistle_util
        vistle_core
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)

add_module(ConnectLibSim ${MODULE_SOURCES} ${MODULE_HEADERS})
target_link_libraries(ConnectLibSim libsimV2runntime_par)


