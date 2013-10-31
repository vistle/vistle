find_package(Boost 1.40 REQUIRED COMPONENTS iostreams filesystem system)

function(add_compile_flags file flags)
   get_property(MY_CFLAGS SOURCE ${file} PROPERTY COMPILE_FLAGS)
   if(NOT MY_CFLAGS)
      set(MY_CFLAGS "")
   endif()
   set(MY_CFLAGS "${MY_CFLAGS} ${flags}")
   set_property(SOURCE ${file} PROPERTY COMPILE_FLAGS "${MY_CFLAGS}")
endfunction(add_compile_flags)

if (CMAKE_COMPILER_IS_GNUCC)
   execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   if (GCC_VERSION VERSION_LESS 4.2)
      message(STATUS "GCC version older than 4.2, not building ReadFOAM")
      return()
   endif()
endif()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

if (CMAKE_COMPILER_IS_GNUCC)
   string(REPLACE "-std=c++0x" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   add_compile_flags(ReadFOAM.cpp -std=c++0x)
endif()

add_executable(ReadFOAM ReadFOAM.cpp foamtoolbox.cpp)
target_link_libraries(ReadFOAM
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)

add_executable(foambounds bounds.cpp foamtoolbox.cpp)
target_link_libraries(foambounds ${Boost_LIBRARIES})
