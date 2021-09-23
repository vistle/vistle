set(LIBSIM_INTERFACE_SOURCES
    ${LIBSIM_INTERFACE_DIR}/CommandMetaData.C
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
    VisItDataInterfaceRuntime.C
    VisItDataInterfaceRuntimeP.C)

set(LIBSIM_INTERFACE_HEADERS
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
    VisItDataInterfaceRuntime.h
    VisItDataInterfaceRuntimeP.h
    VisItDataTypes.h)

set(LIBSIM_INTERFACE_FILES "")

foreach(FILE IN LISTS LIBSIM_INTERFACE_SOURCES LIBSIM_INTERFACE_HEADERS)
    list(APPEND LIBSIM_INTERFACE_FILES "${CMAKE_CURRENT_LIST_DIR}/${FILE}")
endforeach(FILE IN LISTS)
