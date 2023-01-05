vistle_find_package(Qt5Core)

vistle_find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)
if(Qt5_FOUND)
    function(qt_disable_unicode_defines)

    endfunction()
else()
    vistle_find_package(Qt6 COMPONENTS Widgets)
endif()
if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message("Not building GUI: Qt5 Core library not found")
    return()
endif()
add_subdirectory(propertybrowser)
add_subdirectory(remotefilebrowser)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(Python_FOUND)
    add_definitions(-DHAVE_PYTHON)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} .)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/lib/3rdparty/qt-solutions/qtpropertybrowser/src)

set(gui_SOURCES
    main.cpp
    mainwindow.cpp
    connection.cpp
    module.cpp
    dataflownetwork.cpp
    port.cpp
    vistleobserver.cpp
    parameters.cpp
    modulebrowser.cpp
    moduleview.cpp
    dataflowview.cpp
    vistleconsole.cpp
    modifieddialog.cpp
    uicontroller.cpp
    qconsole/qconsole.cpp)

set(gui_HEADERS
    connection.h
    dataflownetwork.h
    dataflowview.h
    macosutils.h
    mainwindow.h
    modifieddialog.h
    module.h
    modulebrowser.h
    moduleview.h
    parameters.h
    port.h
    uicontroller.h
    vistleconsole.h
    vistleobserver.h
    qconsole/qconsole.h)

set(gui_FORMS mainwindow.ui about.ui modulebrowser.ui moduleview.ui modifieddialog.ui)

set(gui_name vistle_gui)
if(APPLE)
    add_custom_target(
        gui_wrapper ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/gui_wrapper.sh ${EXECUTABLE_OUTPUT_PATH}/$<CONFIG>/${gui_name}
        DEPENDS gui_wrapper.sh
        SOURCES gui_wrapper.sh)
    install(
        PROGRAMS gui_wrapper.sh
        DESTINATION bin
        RENAME vistle_gui)
    set(MACOSX_BUNDLE_ICON_FILE vistle.icns)
    set(gui_SOURCES ${gui_SOURCES} ${MACOSX_BUNDLE_ICON_FILE})
    set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(gui_name Vistle)
    set(gui_SOURCES ${gui_SOURCES} macosutils.m)
    set(gui_HEADERS ${gui_SOURCES} macosutils.h)
endif(APPLE)

qt_wrap_ui(GUI_UI_SRCS ${gui_FORMS})
qt_add_resources(GUI_QRC_SOURCES gui.qrc about.qrc)
add_definitions(${QT_DEFINITIONS})
vistle_add_executable(${gui_name} MACOSX_BUNDLE ${gui_SOURCES} ${GUI_UI_SRCS} ${GUI_QRC_SOURCES})
qt_disable_unicode_defines(${gui_name})

target_link_libraries(${gui_name} PRIVATE Qt::Widgets)
target_link_libraries(${gui_name} PRIVATE Threads::Threads)
target_link_libraries(
    ${gui_name}
    PRIVATE ${QT_LIBRARIES}
            Boost::boost
            vistle_util
            vistle_core
            vistle_userinterface
            vistle_pythonmodule
            vistle_propertybrowser
            vistle_remotefiledialog)

if(Python_FOUND)
    target_link_libraries(${gui_name} PRIVATE Python::Python vistle_python)
endif()

if(APPLE)
    target_link_libraries(${gui_name} PRIVATE "-framework AppKit")
endif()
