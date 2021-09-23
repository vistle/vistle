find_package(CFX)
if(NOT CFX_FOUND)
    return()
endif()

include_directories(SYSTEM ${CFX_INCLUDE_DIRS})
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")

add_module(ReadCfx "read ANSYS CFX data" ReadCFX.cpp)

target_link_libraries(ReadCfx ${CFX_LIBRARIES})
