include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_PATH}
        ../core
        ..
)

add_executable(blower blower.cpp)
target_link_libraries(blower
        ${Boost_LIBRARIES}
        vistle_core
        vistle_userinterface
)
