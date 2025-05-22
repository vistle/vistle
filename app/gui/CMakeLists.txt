if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message("Not building GUI: neither Qt5 nor Qt6 Core library found")
    return()
endif()

add_subdirectory(propertybrowser)
add_subdirectory(remotefilebrowser)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(gui_SOURCES
    main.cpp
    mainwindow.cpp
    connection.cpp
    module.cpp
    dataflownetwork.cpp
    port.cpp
    vistleobserver.cpp
    parameters.cpp
    parameterconnectionwidgets.cpp
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
    parameterconnectionwidgets.h
    port.h
    uicontroller.h
    vistleconsole.h
    vistleobserver.h
    qconsole/qconsole.h)

set(gui_FORMS mainwindow.ui about.ui modulebrowser.ui moduleview.ui modifieddialog.ui setnamedialog.ui)

if(APPLE)
    add_custom_target(
        gui_wrapper
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/gui_wrapper.sh ${EXECUTABLE_OUTPUT_PATH}/$<CONFIG>/vistle_gui
        DEPENDS gui_wrapper.sh
        SOURCES gui_wrapper.sh)
    install(
        PROGRAMS gui_wrapper.sh
        DESTINATION bin
        RENAME vistle_gui)
    set(MACOSX_BUNDLE_ICON_FILE vistle.icns)
    set(gui_SOURCES ${gui_SOURCES} ${MACOSX_BUNDLE_ICON_FILE})
    set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(gui_SOURCES ${gui_SOURCES} macosutils.m)
    set(gui_HEADERS ${gui_SOURCES} macosutils.h)
endif(APPLE)

vistle_add_executable(vistle_gui MACOSX_BUNDLE ${gui_SOURCES})
if(VISTLE_USE_QT5)
    qt5_wrap_ui(GUI_UI_SRCS ${gui_FORMS})
    qt5_add_resources(GUI_QRC_SOURCES gui.qrc about.qrc)
    target_link_libraries(vistle_gui PRIVATE Qt5::Widgets)
else()
    qt6_wrap_ui(GUI_UI_SRCS ${gui_FORMS})
    qt6_add_resources(GUI_QRC_SOURCES gui.qrc about.qrc)
    target_link_libraries(vistle_gui PRIVATE Qt6::Widgets)
    qt_disable_unicode_defines(vistle_gui)
endif()
target_sources(vistle_gui PRIVATE ${GUI_UI_SRCS} ${GUI_QRC_SOURCES})
target_link_libraries(vistle_gui PRIVATE Threads::Threads)
target_link_libraries(
    vistle_gui
    PRIVATE Boost::boost
            Boost::program_options
            vistle_util
            vistle_core
            vistle_config
            vistle_userinterface
            vistle_module_descriptions
            vistle_pythonmodule
            vistle_propertybrowser
            vistle_remotefiledialog)

if(Python_FOUND)
    target_compile_definitions(vistle_gui PRIVATE -DHAVE_PYTHON)
    target_link_libraries(vistle_gui PRIVATE Python::Python vistle_python)
endif()

if(APPLE)
    target_link_libraries(vistle_gui PRIVATE "-framework AppKit")
    add_dependencies(vistle_gui gui_wrapper)
    set_target_properties(
        vistle_gui
        PROPERTIES OUTPUT_NAME Vistle
                   MACOSX_BUNDLE_BUNDLE_NAME Vistle
                   MACOSX_BUNDLE_GUI_IDENTIFIER io.vistle.Vistle
                   MACOSX_BUNDLE_ICON_FILE vistle.icns)
endif()
