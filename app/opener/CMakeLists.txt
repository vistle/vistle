set(NAME vistle_opener)

if(APPLE)
    add_custom_target(
        opener_wrapper ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/opener_wrapper.sh ${EXECUTABLE_OUTPUT_PATH}/${NAME}
        DEPENDS opener_wrapper.sh
        SOURCES opener_wrapper.sh)
    install(
        PROGRAMS opener_wrapper.sh
        DESTINATION bin
        RENAME ${NAME})

    vistle_find_package(Qt5Core)
    if(NOT Qt5Core_FOUND)
        return()
    endif()
    vistle_find_package(Qt5Widgets)
    vistle_find_package(Qt5Gui)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    include_directories(${CMAKE_CURRENT_BINARY_DIR} .)
endif()

vistle_add_executable(${NAME} MACOSX_BUNDLE vistle_opener.cpp vistle_opener.h)
target_link_libraries(${NAME} Boost::system Boost::filesystem Boost::program_options Threads::Threads)
target_link_libraries(${NAME} Qt5::Widgets)
set_target_properties(${NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/vistle_opener_info.plist)
