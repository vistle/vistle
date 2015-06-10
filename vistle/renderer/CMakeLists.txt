vistle_find_package(LibVncServer)
vistle_find_package(TBB)

set(renderer_SOURCES
   renderer.cpp
   renderobject.cpp
)

set(renderer_HEADERS
   export.h
   renderer.h
   renderobject.h
)

if(LIBVNCSERVER_FOUND AND TBB_FOUND)
   set(renderer_SOURCES ${renderer_SOURCES}
      vnccontroller.cpp
      parrendmgr.cpp
   )
   set(renderer_HEADERS ${renderer_HEADERS}
      vnccontroller.h
      parrendmgr.h
   )
   include_directories(
      ${TBB_INCLUDE_DIR}
      ${LIBVNCSERVER_INCLUDE_DIR}
      ${ICET_INCLUDE_DIRS}
   )
endif()

use_openmp()

add_library(vistle_renderer ${VISTLE_LIB_TYPE} ${renderer_SOURCES} ${renderer_HEADERS})

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_renderer
		-lrt
	)
endif()

target_link_libraries(vistle_renderer
        vistle_util
        vistle_core
        vistle_module
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)
if(LIBVNCSERVER_FOUND AND TBB_FOUND)
   target_link_libraries(vistle_renderer
      vistle_rhr
      ${ICET_CORE_LIBS}
      ${ICET_MPI_LIBS}
   )
endif()

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


add_subdirectory(COVER)
add_subdirectory(OsgRenderer)
add_subdirectory(DisCOVERay)
