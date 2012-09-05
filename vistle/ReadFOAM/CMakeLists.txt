find_package(MPI REQUIRED)
find_package(Boost 1.42 REQUIRED COMPONENTS date_time iostreams)
find_package(ZLIB REQUIRED)

add_executable(ReadFOAM ReadFOAM.cpp gzip.cpp zlib.cpp)
add_definitions(-DBOOST_IOSTREAMS_NO_LIB)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
)

target_link_libraries(ReadFOAM
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
	${ZLIB_LIBRARY}
        vistle_core
)
