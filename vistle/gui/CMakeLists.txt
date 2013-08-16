project(gui)
set(CMAKE_AUTOMOC ON)

find_qt()

include_directories(
  ${Boost_INCLUDE_DIRS}
  ..
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(gui_SOURCES
  main.cpp
  mainwindow.cpp
  connection.cpp
  module.cpp
  scene.cpp
  port.cpp
  vistleconnection.cpp

  qconsole/qconsole.cpp
  qconsole/qpyconsole.cpp
)

set(gui_HEADERS
  mainwindow.h
  connection.h
  consts.h
  module.h
  scene.h
  port.h
  vistleconnection.h
)

set(gui_FORMS
  mainwindow.ui
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
  vistle_core
  vistle_ui
)
