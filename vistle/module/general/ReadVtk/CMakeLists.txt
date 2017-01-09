vistle_find_package(VTK
   COMPONENTS vtkIOLegacy vtkIOXML vtkFiltersCore vtkCommonCore vtkImagingCore vtkCommonDataModel vtkCommonExecutionModel
   NO_MODULE)

vistle_find_package(TinyXML2)
if (TINYXML2_FOUND)
   add_definitions(-DHAVE_TINYXML2)
   include_directories(${TINYXML2_INCLUDE_DIRS})
endif()

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-inconsistent-missing-override")
endif()

add_module(ReadVtk
   ReadVtk.cpp
   ReadVtk.h
   coVtk.cpp
   coVtk.h
)

include_directories(
   ${VTK_INCLUDE_DIRS}
   ${Boost_INCLUDE_DIRS}
   ${MPI_C_INCLUDE_PATH}
   ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ReadVtk
   ${Boost_LIBRARIES}
   ${MPI_C_LIBRARIES}
   ${VTK_LIBRARIES}
   vistle_module
)

if (TINYXML2_FOUND)
   target_link_libraries(ReadVtk ${TINYXML2_LIBRARIES})
endif()
