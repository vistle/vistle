set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(SYSTEM
   ${Boost_INCLUDE_DIRS}
   ../../../3rdparty/qt-solutions/qtpropertybrowser/src
)
include_directories(
   ../..
   ${CMAKE_CURRENT_BINARY_DIR}
)
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
   vistlefileinfogatherer.cpp
)

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
   vistlefileinfogatherer.h
)

set(FORMS
   remotefiledialog.ui
)

qt_wrap_ui(UI_SRCS ${FORMS})
add_definitions(${QT_DEFINITIONS})
add_definitions("-DQT_DISABLE_DEPRECATED_BEFORE=0x000000")
add_library(vistle_remotefiledialog STATIC
  ${SOURCES} 
  ${UI_SRCS})

qt_use_modules(vistle_remotefiledialog Widgets)

target_link_libraries(vistle_remotefiledialog
  ${QT_LIBRARIES}
  vistle_core
)
