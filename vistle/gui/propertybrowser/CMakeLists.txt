set(CMAKE_AUTOMOC ON)



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
   vistlebrowserfactory.cpp
   qtvectorpropertymanager.cpp
   qtlongvectorpropertymanager.cpp
   vistledoublepropertymanager.cpp
   vistlebrowserpropertymanager.cpp

   vistlebrowseredit.cpp
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
   vistlebrowserfactory.h
   qtvectorpropertymanager.h
   qtlongvectorpropertymanager.h
   vistledoublepropertymanager.h
   vistlebrowserpropertymanager.h

   vistlebrowseredit.h
)

set(FORMS
)

qt5_wrap_ui(UI_SRCS ${FORMS})

add_library(vistle_propertybrowser STATIC
  ${SOURCES} 
  ${UI_SRCS})

target_compile_definitions(vistle_propertybrowser PRIVATE ${QT_DEFINITIONS})
target_compile_definitions(vistle_propertybrowser PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)


target_include_directories(vistle_propertybrowser SYSTEM
   #PRIVATE ${Boost_INCLUDE_DIRS}
   PRIVATE ../../../3rdparty/qt-solutions/qtpropertybrowser/src
)
target_include_directories(vistle_propertybrowser
   PRIVATE ../..
   PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(vistle_propertybrowser
    PRIVATE ${QT_LIBRARIES} vistle_core Qt5::Widgets
)
