set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(SOURCES
    remotefiledialog.cpp
    filedialoghelper.cpp
    qsidebar.cpp
    abstractfilesystemmodel.cpp
    #localfilesystemmodel.cpp
    remotefilesystemmodel.cpp
    abstractfileinfogatherer.cpp
    remotefileiconprovider.cpp
    remotefileinfogatherer.cpp
    vistlefileinfogatherer.cpp)

set(HEADERS
    remotefiledialog.h
    remotefiledialog_p.h
    filedialoghelper.h
    qsidebar_p.h
    remotefscompleter_p.h
    abstractfilesystemmodel.h
    #localfilesystemmodel.h
    remotefilesystemmodel.h
    remotefilesystemmodel_p.h
    abstractfileinfogatherer.h
    remotefileiconprovider.h
    remotefileinfogatherer.h
    vistlefileinfogatherer.h)

set(FORMS remotefiledialog.ui)

qt_wrap_ui(UI_SRCS ${FORMS})

add_library(vistle_remotefiledialog STATIC ${SOURCES} ${UI_SRCS})

target_compile_definitions(vistle_remotefiledialog PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)

target_include_directories(vistle_remotefiledialog SYSTEM PRIVATE ../../../3rdparty/qt-solutions/qtpropertybrowser/src)
target_include_directories(
    vistle_remotefiledialog
    PRIVATE ../..
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vistle_remotefiledialog PRIVATE vistle_core)
if(VISTLE_USE_QT5)
    target_link_libraries(vistle_remotefiledialog PRIVATE Qt5::Widgets)
else()
    target_link_libraries(vistle_remotefiledialog PRIVATE Qt6::Widgets)
    qt_disable_unicode_defines(vistle_remotefiledialog)
endif()
