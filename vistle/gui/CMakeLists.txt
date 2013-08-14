PROJECT(gui)
SET(CMAKE_AUTOMOC ON)

find_qt()

INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIRS}
  ..
  ${CMAKE_CURRENT_BINARY_DIR}
)

SET(gui_SOURCES
  main.cpp
  mainwindow.cpp
  connection.cpp
  module.cpp
  scene.cpp
  port.cpp
  handler.cpp
)

SET(gui_HEADERS
  mainwindow.h
  connection.h
  consts.h
  module.h
  scene.h
  port.h
  handler.h
)

SET(gui_FORMS
  mainwindow.ui
)

SET(gui_RESOURCES
  input.txt
  module_list.txt
)

QT_WRAP_UI(GUI_UI_SRCS ${gui_FORMS})
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_EXECUTABLE(gui 
  ${gui_SOURCES} 
  ${GUI_UI_SRCS})

qt5_use_modules(gui Widgets)

TARGET_LINK_LIBRARIES(gui 
  ${QT_LIBRARIES}
  ${Boost_LIBRARIES}
  vistle_core
  vistle_ui
)
