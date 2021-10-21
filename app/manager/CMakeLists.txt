# Vistle cleaner
vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle Boost::system MPI::MPI_C vistle_core vistle_util_mpi ${BOOST_MPI})

# Vistle manager
set(SOURCES vistle_manager.cpp)

if(NOT VISTLE_MULTI_PROCESS)

endif()

set(LIB_SOURCES
    manager.cpp
    clustermanager.cpp
    datamanager.cpp
    communicator.h
    communicator.cpp
    executor.cpp
    portmanager.cpp)

set(LIB_HEADERS export.h clustermanager.h datamanager.h manager.h portmanager.h run_on_main_thread.h)

if(NOT VISTLE_MULTI_PROCESS)
    vistle_add_library(vistle_clustermanager ${LIB_SOURCES} ${LIB_HEADERS})
    vistle_export_library(vistle_clustermanager ${LIB_SOURCES} ${LIB_HEADERS})

    if(COVER_FOUND)
        vistle_find_package(XCB)
        vistle_find_package(X11)
        vistle_find_package(Qt5Core)
        vistle_find_package(Qt5Widgets)
        if(Qt5Widgets_FOUND)
            if(APPLE
               OR WIN32
               OR XCB_FOUND)
                set(USE_QT TRUE)
                target_compile_definitions(vistle_clustermanager PRIVATE HAVE_QT)
                if(X11_ICE_FOUND)
                    target_compile_definitions(vistle_clustermanager PRIVATE HAVE_X11_ICE)
                    target_include_directories(vistle_clustermanager PRIVATE X11_ICE_INCLUDE_PATH)
                endif()
            endif()
        endif()
    endif()
    target_link_libraries(
        vistle_clustermanager
        ${BOOST_MPI}
        Boost::system
        MPI::MPI_C
        vistle_control
        vistle_util
        vistle_util_mpi
        vistle_core
        Threads::Threads)
endif()

if(USE_QT)
    qt5_add_resources(QRC_SOURCES icons.qrc)
    set(LIB_SOURCES ${LIB_SOURCES} ${QRC_SOURCES})
endif()

if(VISTLE_MULTI_PROCESS)
    set(SOURCES ${SOURCES} ${LIB_SOURCES})
    set(HEADERS ${HEADERS} ${LIB_HEADERS})
endif()

vistle_add_executable(vistle_manager ${SOURCES})

target_link_libraries(
    vistle_manager
    ${BOOST_MPI}
    Boost::system
    MPI::MPI_C
    vistle_control
    vistle_util
    vistle_util_mpi
    vistle_core
    Threads::Threads)

if(NOT VISTLE_MULTI_PROCESS)
    target_link_libraries(vistle_clustermanager ${CMAKE_DL_LIBS} vistle_module)
    if(APPLE)
        set_target_properties(vistle_clustermanager PROPERTIES LINK_FLAGS -Wl,-export_dynamic)
    endif()
    if(USE_QT)
        target_link_libraries(vistle_clustermanager Qt5::Core Qt5::Widgets)
        if(NOT APPLE
           AND NOT WIN32
           AND XCB_FOUND)
            target_link_libraries(vistle_clustermanager ${XCB_LIBRARIES})
            if(X11_ICE_FOUND)
                target_link_libraries(vistle_clustermanager ${X11_ICE_LIB})
            endif()
        endif()
    endif()
    target_link_libraries(vistle_manager vistle_clustermanager)

    if(NOT VISTLE_MODULES_SHARED)
        use_openmp()
        foreach(m ${ALL_MODULES})
            if(APPLE)
                target_link_libraries(vistle_manager -Wl,-force_load ${m})
            else()
                target_link_libraries(vistle_manager -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
            endif()
            #target_link_libraries(vistle_clustermanager $<TARGET_OBJECTS:${m}>)
        endforeach()
        target_link_libraries(vistle_manager vistle_module_descriptions)
    endif()
endif()
