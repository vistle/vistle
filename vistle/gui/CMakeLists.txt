if (NOT ${VISTLE_GUI})
   return()
endif()

find_qt()

add_subdirectory(propertybrowser)

find_package(PythonLibs)

set(CMAKE_AUTOMOC ON)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${PYTHON_INCLUDE_PATH}
  ..
  ${CMAKE_CURRENT_BINARY_DIR}
  .
  ../../3rdparty/qt-solutions/qtpropertybrowser/src
)

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
  dataflowview.cpp
  vistleconsole.cpp
  modifieddialog.cpp
  uicontroller.cpp

  qconsole/qconsole.cpp
)

set(gui_HEADERS
  mainwindow.h
  connection.h
  module.h
  dataflownetwork.h
  port.h
  vistleobserver.h
  parameters.h
  modulebrowser.h
  dataflowview.h
  vistleconsole.h
  modifieddialog.h
  uicontroller.h

  qconsole/qconsole.h
)

set(gui_FORMS
  mainwindow.ui
  modulebrowser.ui
  modifieddialog.ui
)

qt_wrap_ui(GUI_UI_SRCS ${gui_FORMS})
add_definitions(${QT_DEFINITIONS})
add_executable(gui 
  ${gui_SOURCES} 
  ${GUI_UI_SRCS})

qt5_use_modules(gui Widgets)

target_link_libraries(gui 
  ${QT_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  vistle_core
  vistle_userinterface
  vistle_propertybrowser
)
