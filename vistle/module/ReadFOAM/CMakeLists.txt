find_package(Boost 1.42 REQUIRED COMPONENTS iostreams)
find_package(ZLIB REQUIRED)

if (CMAKE_COMPILER_IS_GNUCC)
   execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   if (GCC_VERSION VERSION_LESS 4.2)
      message(STATUS "GCC version older than 4.2, not building ReadFOAM")
      return()
   endif()
endif()

add_executable(ReadFOAM ReadFOAM.cpp gzip.cpp zlib.cpp)
add_definitions(-DBOOST_IOSTREAMS_NO_LIB)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}/core
)

target_link_libraries(ReadFOAM
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${ZLIB_LIBRARY}
        vistle_core
)
