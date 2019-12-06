vistle_find_package(XCB)
vistle_find_package(X11)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ..
)

if (VISTLE_SHM_PER_RANK)
   add_definitions(-DSHMPERRANK)
endif()

# Vistle cleaner
vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_SYSTEM_LIBRARY}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)

# Vistle manager
set(SOURCES
   vistle_manager.cpp
)

if (NOT VISTLE_MULTI_PROCESS)
    set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    if (APPLE)
        set(CMAKE_PREFIX_PATH /usr/local/opt/qt5 ${CMAKE_PREFIX_PATH})
    endif()
    set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/qt5 ${CMAKE_PREFIX_PATH})
    vistle_find_package(Qt5Core)
    vistle_find_package(Qt5Widgets)
    if (Qt5Widgets_FOUND)
        if (APPLE OR WIN32 OR XCB_FOUND)
            set(USE_QT TRUE)
            add_definitions(-DHAVE_QT)
            if (X11_ICE_FOUND)
                include_directories(X11_ICE_INCLUDE_PATH)
            endif()
        endif()
    endif()
endif()

set(LIB_SOURCES
   manager.cpp
   clustermanager.cpp
   datamanager.cpp
   communicator.h
   communicator.cpp
   executor.cpp
   portmanager.cpp
   run_on_main_thread.cpp
)

set(LIB_HEADERS
    export.h
   clustermanager.h
   datamanager.h
   datamanager.h
   manager.h
   portmanager.h
   run_on_main_thread.h
)

if (USE_QT)
   qt_add_resources(QRC_SOURCES icons.qrc)
   set(LIB_SOURCES ${LIB_SOURCES} ${QRC_SOURCES})
endif()

if (NOT VISTLE_MULTI_PROCESS)
vistle_add_library(vistle_clustermanager ${LIB_SOURCES} ${LIB_HEADERS})

target_link_libraries(vistle_clustermanager
        vistle_boost_mpi
        ${Boost_SYSTEM_LIBRARY}
        ${MPI_C_LIBRARIES}
        vistle_control
        vistle_util
        vistle_core
        vistle_boost_mpi
)
endif()

if(VISTLE_MULTI_PROCESS)
    set(SOURCES ${SOURCES} ${LIB_SOURCES})
    set(HEADERS ${HEADERS} ${LIB_HEADERS})
endif()

if(NOT VISTLE_MULTI_PROCESS)
   target_link_libraries(vistle_clustermanager ${CMAKE_DL_LIBS} vistle_module)
   if (NOT VISTLE_MODULES_SHARED)
      use_openmp()
      foreach(m ${ALL_MODULES})
         if (APPLE)
            target_link_libraries(vistle_clustermanager -Wl,-force_load ${m})
         else()
            target_link_libraries(vistle_clustermanager -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
         endif()
         #target_link_libraries(vistle_clustermanager $<TARGET_OBJECTS:${m}>)
      endforeach()
   endif()
   if (APPLE)
      set_target_properties(vistle_clustermanager PROPERTIES LINK_FLAGS -Wl,-export_dynamic)
   endif()
   if (USE_QT)
      qt_use_modules(vistle_clustermanager Core Widgets)
      if (NOT APPLE AND NOT WIN32 AND XCB_FOUND)
          target_link_libraries(vistle_clustermanager ${XCB_LIBRARIES})
          if (X11_ICE_FOUND)
              target_link_libraries(vistle_clustermanager ${X11_ICE_LIB})
          endif()
      endif()
   endif()
endif()

vistle_add_executable(vistle_manager ${SOURCES})

target_link_libraries(vistle_manager
		vistle_boost_mpi
        ${Boost_SYSTEM_LIBRARY}
        ${MPI_C_LIBRARIES}
        vistle_control
        vistle_util
        vistle_core
        vistle_boost_mpi
)

if (NOT VISTLE_MULTI_PROCESS)
    target_link_libraries(vistle_manager
        vistle_clustermanager
    )
endif()
