

SET(LIBSIM_SOURCES 
  CommandMetaData.C
  CSGMesh.C
  CurveData.C
  CurveMetaData.C
  CurvilinearMesh.C
  DomainList.C 
  ExpressionMetaData.C
  MaterialData.C
  MaterialMetaData.C
  MeshMetaData.C
  MessageMetaData.C
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
  #VisItControlInterfaceRuntime.C 
  VisItDataInterfaceRuntime.C 
  VisItDataInterfaceRuntimeP.C 

)

SET(LIBSIM_HEADERS 
  CommandMetaData.h
  CSGMesh.h
  CurveData.h
  CurveMetaData.h
  CurvilinearMesh.h
  DomainList.h 
  export.h
  ExpressionMetaData.h
  MaterialData.h
  MaterialMetaData.h
  MeshMetaData.h
  MessageMetaData.h
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
  VectorTypes.h
  #VisItControlInterfaceRuntime.h
  VisItDataInterfaceRuntime.h
  VisItDataInterfaceRuntimeP.h
  VisItDataTypes.h

)



if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

vistle_add_library(vistle_libsim STATIC ${LIBSIM_HEADERS} ${LIBSIM_SOURCES})
vistle_export_library(vistle_libsim ${LIBSIM_HEADERS} ${LIBSIM_SOURCES})



