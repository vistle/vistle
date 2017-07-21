set(BOOST_COMPONENTS iostreams filesystem system)
if (WIN32)
   set(BOOST_COMPONENTS ${BOOST_COMPONENTS} zlib)
endif()
vistle_find_package(Boost 1.53 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
vistle_find_package(ZLIB)

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
      message(STATUS "GCC version older than 4.2, not building ReadFoam")
      return()
   endif()
endif()

include_directories(
        ../../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

add_module(ReadFoam ReadFOAM.cpp foamtoolbox.cpp)
target_link_libraries(ReadFoam
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        ${ZLIB_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)

#add_executable(foambounds bounds.cpp foamtoolbox.cpp)
#target_link_libraries(foambounds ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})
