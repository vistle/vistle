
find_package(CFX)
if(NOT CFX_FOUND)
return()
endif()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
	${CFX_INCLUDE_DIRS}
)

add_module(ReadCFX ReadCFX.cpp)

target_link_libraries(ReadCFX
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
	${CFX_LIBRARIES}
        vistle_module
)
