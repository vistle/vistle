if(NOT VTK_FOUND)
    return()
endif()

set(VTK_DEPENDENCIES "\nfind_dependency(
    VTK
    5.0
    COMPONENTS
    vtkIOLegacy
    vtkIOXML
    vtkFiltersCore
    vtkCommonCore
    vtkImagingCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    NO_MODULE
    QUIET)")

set(SOURCES vtktovistle.cpp)

set(HEADERS export.h vtktovistle.h)

#vistle_vtk
vistle_add_library(vistle_vtk EXPORT ${SOURCES} ${HEADERS})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(vistle_vtk PRIVATE -Wno-deprecated)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    target_compile_options(vistle_vtk PRIVATE -Wno-deprecated -Wno-inconsistent-missing-override)
endif()

target_include_directories(vistle_vtk SYSTEM PUBLIC ${VTK_INCLUDE_DIRS})

vistle_target_link_libraries(
    vistle_vtk
    PRIVATE
    Threads::Threads
    PUBLIC
    ${VTK_LIBRARIES}
    PUBLIC
    vistle_core)
target_compile_definitions(vistle_vtk PUBLIC ${VTK_DEFINITIONS})
file(APPEND ${buildPackageLocation}/vistle_vtkConfig.cmake ${VTK_DEPENDENCIES})

#vistle_insitu_vtk
vistle_add_library(vistle_sensei_vtk EXPORT ${SOURCES} ${HEADERS})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(vistle_sensei_vtk PRIVATE -Wno-deprecated)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    target_compile_options(vistle_sensei_vtk PRIVATE -Wno-deprecated -Wno-inconsistent-missing-override)
endif()



target_include_directories(vistle_sensei_vtk SYSTEM PUBLIC ${VTK_INCLUDE_DIRS})
vistle_target_link_libraries(
    vistle_sensei_vtk
    PRIVATE
    Threads::Threads
    PUBLIC
    ${VTK_LIBRARIES}
    PUBLIC
    vistle_core
    PUBLIC
    vistle_sensei)
target_compile_definitions(
    vistle_sensei_vtk
    PUBLIC ${VTK_DEFINITIONS}
    PUBLIC SENSEI)


file(APPEND ${buildPackageLocation}/vistle_sensei_vtkConfig.cmake ${VTK_DEPENDENCIES})