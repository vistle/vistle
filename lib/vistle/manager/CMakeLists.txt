set(LIB_SOURCES
    manager.cpp
    clustermanager.cpp
    datamanager.cpp
    communicator.h
    communicator.cpp
    executor.cpp
    portmanager.cpp)

set(LIB_HEADERS export.h clustermanager.h datamanager.h manager.h portmanager.h run_on_main_thread.h)

if(VISTLE_MULTI_PROCESS)
    vistle_add_library(vistle_clustermanager INTERNAL STATIC ${LIB_SOURCES} ${LIB_HEADERS})
else()
    vistle_add_library(vistle_clustermanager EXPORT ${LIB_SOURCES} ${LIB_HEADERS})

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
endif()

if(USE_QT)
    qt5_add_resources(QRC_SOURCES icons.qrc)
    set(LIB_SOURCES ${LIB_SOURCES} ${QRC_SOURCES})
endif()

vistle_target_link_libraries(
    vistle_clustermanager
    PRIVATE ${BOOST_MPI}
    PRIVATE Boost::system
    PRIVATE MPI::MPI_C
    PRIVATE vistle_control
    PUBLIC vistle_util
    PRIVATE vistle_util_mpi
    PUBLIC vistle_core
    PRIVATE Threads::Threads)

if(NOT VISTLE_MULTI_PROCESS)
    vistle_target_link_libraries(vistle_clustermanager PRIVATE ${CMAKE_DL_LIBS} PRIVATE vistle_module)
    if(APPLE)
        set_target_properties(vistle_clustermanager PROPERTIES LINK_FLAGS -Wl,-export_dynamic)
    endif()
    if(USE_QT)
        target_link_libraries(vistle_clustermanager PRIVATE Qt5::Core Qt5::Widgets)
        if(NOT APPLE
           AND NOT WIN32
           AND XCB_FOUND)
            target_link_libraries(vistle_clustermanager PRIVATE ${XCB_LIBRARIES})
            if(X11_ICE_FOUND)
                target_link_libraries(vistle_clustermanager PRIVATE ${X11_ICE_LIB})
            endif()
        endif()
    endif()
endif()
