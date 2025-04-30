set(CMAKE_AUTOMOC TRUE)

set(SOURCES
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtbuttonpropertybrowser.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qteditorfactory.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtgroupboxpropertybrowser.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowser.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowserutils.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertymanager.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qttreepropertybrowser.cpp
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtvariantproperty.cpp
    qlongspinbox.cpp
    qtlongpropertymanager.cpp
    qtlongeditorfactory.cpp
    vistlebuttonpropertybrowser.cpp
    vistledoubleeditorfactory.cpp
    vistlelineeditfactory.cpp
    vistlebrowserfactory.cpp
    qtvectorpropertymanager.cpp
    qtlongvectorpropertymanager.cpp
    vistledoublepropertymanager.cpp
    vistlebrowserpropertymanager.cpp
    vistlebrowseredit.cpp)

set(HEADERS
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtbuttonpropertybrowser.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qteditorfactory.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtgroupboxpropertybrowser.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowser.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowserutils_p.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertymanager.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qttreepropertybrowser.h
    ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src/qtvariantproperty.h
    qlongspinbox.h
    qtlongpropertymanager.h
    qtlongeditorfactory.h
    vistlebuttonpropertybrowser.h
    vistledoubleeditorfactory.h
    vistlelineeditfactory.h
    vistlebrowserfactory.h
    qtvectorpropertymanager.h
    qtlongvectorpropertymanager.h
    vistledoublepropertymanager.h
    vistlebrowserpropertymanager.h
    vistlebrowseredit.h)

set(FORMS)

add_library(vistle_propertybrowser STATIC ${SOURCES} ${HEADERS})
if(VISTLE_USE_QT5)
    qt5_wrap_ui(UI_SRCS ${FORMS})
    target_link_libraries(vistle_propertybrowser PRIVATE Qt5::Widgets)
else()
    qt6_wrap_ui(UI_SRCS ${FORMS})
    qt_disable_unicode_defines(vistle_propertybrowser)
    target_link_libraries(vistle_propertybrowser PRIVATE Qt6::Widgets)
endif()
target_sources(vistle_propertybrowser PRIVATE ${UI_SRCS})

target_compile_definitions(vistle_propertybrowser PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)

target_include_directories(vistle_propertybrowser SYSTEM PUBLIC ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src)
target_include_directories(vistle_propertybrowser PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vistle_propertybrowser PRIVATE vistle_util vistle_core)
