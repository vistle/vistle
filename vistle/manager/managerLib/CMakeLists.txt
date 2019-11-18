vistle_find_package(XCB)
vistle_find_package(X11)

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


set(SOURCES
   manager.cpp
   clustermanager.cpp
   datamanager.cpp
   communicator.h
   communicator.cpp
   executor.cpp
   portmanager.cpp
)
set(HEADERS
   clustermanager.h
   datamanager.h
   datamanager.h
   manager.h
   portmanager.h
)

if (USE_QT)
   qt_add_resources(QRC_SOURCES ../icons.qrc)
   set(SOURCES ${SOURCES} ${QRC_SOURCES})
endif()

vistle_add_library(vistle_manager_lib ${SOURCES} ${HEADERS})

target_link_libraries(vistle_manager_lib
        vistle_boost_mpi
        ${Boost_SYSTEM_LIBRARY}
        ${MPI_C_LIBRARIES}
        vistle_control
        vistle_util
        vistle_core
        vistle_boost_mpi
)
if(VISTLE_MULTI_PROCESS)
	target_link_libraries(vistle_manager_lib run_on_main_thread)
endif()

if(NOT VISTLE_MULTI_PROCESS)
   target_link_libraries(vistle_manager_lib ${CMAKE_DL_LIBS} vistle_module)
   if (NOT VISTLE_MODULES_SHARED)
      use_openmp()
      foreach(m ${ALL_MODULES})
         if (APPLE)
            target_link_libraries(vistle_manager_lib -Wl,-force_load ${m})
         else()
            target_link_libraries(vistle_manager_lib -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
         endif()
         #target_link_libraries(vistle_manager_lib $<TARGET_OBJECTS:${m}>)
      endforeach()
   endif()
   if (APPLE)
      set_target_properties(vistle_manager_lib PROPERTIES LINK_FLAGS -Wl,-export_dynamic)
   endif()
   if (USE_QT)
      qt_use_modules(vistle_manager_lib Core Widgets)
      if (NOT APPLE AND NOT WIN32 AND XCB_FOUND)
          target_link_libraries(vistle_manager_lib ${XCB_LIBRARIES})
          if (X11_ICE_FOUND)
              target_link_libraries(vistle_manager_lib ${X11_ICE_LIB})
          endif()
      endif()
   endif()
endif()



