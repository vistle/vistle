vistle_find_package(Boost 1.53 REQUIRED COMPONENTS system filesystem)

if (NOT VISTLE_MULTI_PROCESS)
    set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    if (APPLE)
        set(CMAKE_PREFIX_PATH /usr/local/opt/qt5 ${CMAKE_PREFIX_PATH})
    endif()
    set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/qt5 ${CMAKE_PREFIX_PATH})
    vistle_find_package(Qt5Core)
    vistle_find_package(Qt5Widgets)
    if (Qt5Widgets_FOUND)
        set(USE_QT TRUE)
        add_definitions(-DHAVE_QT)
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
   vistle_manager.cpp
   clustermanager.h
   clustermanager.cpp
   datamanager.h
   datamanager.cpp
   communicator.h
   communicator.cpp
   executor.h
   executor.cpp
   portmanager.h
   portmanager.cpp
)

if (USE_QT)
   qt_add_resources(QRC_SOURCES icons.qrc)
   set(SOURCES ${SOURCES} ${QRC_SOURCES})
endif()

vistle_add_executable(vistle_manager ${SOURCES})

target_link_libraries(vistle_manager
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)
if(NOT VISTLE_MULTI_PROCESS)
   target_link_libraries(vistle_manager ${CMAKE_DL_LIBS} vistle_module)
   if (NOT VISTLE_MODULES_SHARED)
      use_openmp()
      foreach(m ${ALL_MODULES})
         if (APPLE)
            target_link_libraries(vistle_manager -Wl,-force_load ${m})
         else()
            target_link_libraries(vistle_manager -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
         endif()
         #target_link_libraries(vistle_manager $<TARGET_OBJECTS:${m}>)
      endforeach()
   endif()
   if (APPLE)
      set_target_properties(vistle_manager PROPERTIES LINK_FLAGS -Wl,-export_dynamic)
   endif()
   if (USE_QT)
      qt_use_modules(vistle_manager Core Widgets)
   endif()
endif()

vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)
