set(CMAKE_AUTOMOC ON)

include_directories(
   ${Boost_INCLUDE_DIRS}
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src
   ../..
   ${CMAKE_CURRENT_BINARY_DIR}
)

set(SOURCES
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtbuttonpropertybrowser.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qteditorfactory.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtgroupboxpropertybrowser.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowser.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowserutils.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertymanager.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qttreepropertybrowser.cpp
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtvariantproperty.cpp

   qlongspinbox.cpp
   qtlongpropertymanager.cpp
   qtlongeditorfactory.cpp
   vistledoubleeditorfactory.cpp
   vistlelineeditfactory.cpp
   qtvectorpropertymanager.cpp
   qtlongvectorpropertymanager.cpp
   vistledoublepropertymanager.cpp
)

set(HEADERS
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtbuttonpropertybrowser.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qteditorfactory.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtgroupboxpropertybrowser.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowser.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertybrowserutils_p.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtpropertymanager.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qttreepropertybrowser.h
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src/qtvariantproperty.h

   qlongspinbox.h
   qtlongpropertymanager.h
   qtlongeditorfactory.h
   vistledoubleeditorfactory.h
   vistlelineeditfactory.h
   qtvectorpropertymanager.h
   qtlongvectorpropertymanager.h
   vistledoublepropertymanager.h
)

set(FORMS
)

qt_wrap_ui(UI_SRCS ${FORMS})
add_definitions(${QT_DEFINITIONS})
add_definitions("-DQT_DISABLE_DEPRECATED_BEFORE=0x000000")
add_library(vistle_propertybrowser STATIC
  ${SOURCES} 
  ${UI_SRCS})

qt_use_modules(vistle_propertybrowser Widgets)

target_link_libraries(vistle_propertybrowser
  ${QT_LIBRARIES}
  vistle_core
)
