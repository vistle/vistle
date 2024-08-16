set(CMAKE_AUTOMOC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

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
    vistledoubleeditorfactory.h
    vistlelineeditfactory.h
    vistlebrowserfactory.h
    qtvectorpropertymanager.h
    qtlongvectorpropertymanager.h
    vistledoublepropertymanager.h
    vistlebrowserpropertymanager.h
    vistlebrowseredit.h)

set(FORMS)

if(VISTLE_USE_QT5)
    qt5_wrap_ui(UI_SRCS ${FORMS})
else()
    qt6_wrap_ui(UI_SRCS ${FORMS})
endif()

add_library(vistle_propertybrowser STATIC ${SOURCES} ${UI_SRCS})

target_compile_definitions(vistle_propertybrowser PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)

target_include_directories(vistle_propertybrowser SYSTEM #PRIVATE ${Boost_INCLUDE_DIRS}
                           PRIVATE ../../../lib/3rdparty/qt-solutions/qtpropertybrowser/src)
target_include_directories(
    vistle_propertybrowser
    PRIVATE ../..
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(VISTLE_USE_QT5)
    target_link_libraries(vistle_propertybrowser PRIVATE ${QT_LIBRARIES} vistle_util vistle_core Qt5::Widgets)
else()
    target_link_libraries(vistle_propertybrowser PRIVATE ${QT_LIBRARIES} vistle_util vistle_core Qt6::Widgets)
    qt_disable_unicode_defines(vistle_propertybrowser)
endif()
