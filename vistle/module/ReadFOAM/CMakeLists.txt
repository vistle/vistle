find_package(Boost 1.42 REQUIRED COMPONENTS iostreams filesystem system)

use_openmp()

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

add_executable(ReadFOAM ReadFOAM.cpp foamtoolbox.cpp)
target_link_libraries(ReadFOAM
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)

add_executable(foambounds bounds.cpp foamtoolbox.cpp)
target_link_libraries(foambounds ${Boost_LIBRARIES})
