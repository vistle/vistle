find_qt()
find_package(PythonLibs)

set(CMAKE_AUTOMOC ON)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${PYTHON_INCLUDE_PATH}
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
  ${PYTHON_LIBRARIES}
  vistle_core
  vistle_ui
)
