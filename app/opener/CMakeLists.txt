set(NAME vistle_opener)

if(APPLE)
    add_custom_target(
        opener_wrapper ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/opener_wrapper.sh ${EXECUTABLE_OUTPUT_PATH}/$<CONFIG>/${NAME}
        DEPENDS opener_wrapper.sh
        SOURCES opener_wrapper.sh)
    install(
        PROGRAMS opener_wrapper.sh
        DESTINATION bin
        RENAME ${NAME})
endif()

if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    #message("Not building vistle_opener: neither Qt5 nor Qt6 Core library found")
    return()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR} .)

vistle_add_executable(${NAME} MACOSX_BUNDLE vistle_opener.cpp vistle_opener.h)
set_target_properties(
    ${NAME}
    PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME vistle_opener
               MACOSX_BUNDLE_GUI_IDENTIFIER io.vistle.vistle_opener
               MACOSX_BUNDLE_ICON_FILE ../gui/vistle.icns)
target_link_libraries(${NAME} PRIVATE vistle_util)
target_link_libraries(${NAME} PRIVATE Boost::system Boost::filesystem Boost::program_options Threads::Threads)
if(VISTLE_USE_QT5)
    target_link_libraries(${NAME} PRIVATE Qt5::Widgets)
else()
    target_link_libraries(${NAME} PRIVATE Qt::Widgets)
endif()
set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/vistle_opener_info.plist)
