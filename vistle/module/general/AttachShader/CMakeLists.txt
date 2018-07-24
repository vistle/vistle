include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)

add_module(AttachShader AttachShader.cpp)

target_link_libraries(AttachShader
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)

