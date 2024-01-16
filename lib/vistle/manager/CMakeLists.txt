set(LIB_SOURCES
    manager.cpp
    clustermanager.cpp
    datamanager.cpp
    communicator.h
    communicator.cpp
    executor.cpp
    portmanager.cpp)

set(LIB_HEADERS
    clustermanager.h
    communicator.h
    datamanager.h
    executor.h
    export.h
    manager.h
    portmanager.h
    run_on_main_thread.h)

vistle_add_library(vistle_clustermanager EXPORT ${LIB_SOURCES} ${LIB_HEADERS})

if(NOT VISTLE_MULTI_PROCESS)
    vistle_find_package(COVER)

    if(COVER_FOUND)
        vistle_find_package(XCB)
        vistle_find_package(X11)

        # message("COVER found, using Qt5: ${COVISE_USE_QT5}")
        if(APPLE
           OR WIN32
           OR XCB_FOUND)
            if(COVISE_USE_QT5)
                vistle_find_package(Qt5 5.15 COMPONENTS Core Widgets)

                if(Qt5_FOUND)
                    set(USE_QT TRUE)
                endif()
            else()
                vistle_find_package(Qt6 COMPONENTS Core Widgets)

                if(Qt6_FOUND)
                    set(USE_QT TRUE)
                endif()
            endif()
        endif()

        if(USE_QT)
            target_compile_definitions(vistle_clustermanager PRIVATE HAVE_QT)

            if(X11_ICE_FOUND)
                target_compile_definitions(vistle_clustermanager PRIVATE HAVE_X11_ICE)
                target_include_directories(vistle_clustermanager PRIVATE X11_ICE_INCLUDE_PATH)
            endif()
        endif()
    endif()
endif()

if(USE_QT)
    if(COVISE_USE_QT5)
        qt5_add_resources(QRC_SOURCES icons.qrc)
    else()
        qt_add_resources(QRC_SOURCES icons.qrc)
    endif()

    set(LIB_SOURCES ${LIB_SOURCES} ${QRC_SOURCES})
endif()

vistle_target_link_libraries(
    vistle_clustermanager
    PRIVATE
    ${BOOST_MPI}
    PRIVATE
    Boost::system
    PRIVATE
    MPI::MPI_C
    PRIVATE
    vistle_control
    PUBLIC
    vistle_util
    PRIVATE
    vistle_util_mpi
    PUBLIC
    vistle_core
    PRIVATE
    Threads::Threads
    PRIVATE
    vistle_module)

if(NOT VISTLE_MODULE_SHARED)
    vistle_target_link_libraries(vistle_clustermanager PRIVATE ${CMAKE_DL_LIBS})

    if(APPLE)
        set_target_properties(vistle_clustermanager PROPERTIES LINK_FLAGS -Wl,-export_dynamic)
    endif()

    if(USE_QT)
        if(COVISE_USE_QT5)
            vistle_target_link_libraries(vistle_clustermanager PRIVATE Qt5::Widgets)
        else()
            vistle_target_link_libraries(vistle_clustermanager PRIVATE Qt6::Widgets)
        endif()

        if(NOT APPLE
           AND NOT WIN32
           AND XCB_FOUND)
            vistle_target_link_libraries(vistle_clustermanager PRIVATE ${XCB_LIBRARIES})

            if(X11_ICE_FOUND)
                vistle_target_link_libraries(vistle_clustermanager PRIVATE ${X11_ICE_LIB})
            endif()
        endif()
    endif()
endif()
