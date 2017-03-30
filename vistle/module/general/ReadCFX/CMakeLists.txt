
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
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

add_module(ReadCFX ReadCFX.cpp)

target_link_libraries(ReadCFX
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
	${CFX_LIBRARIES}
        vistle_module
)
