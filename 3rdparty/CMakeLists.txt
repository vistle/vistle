project(vistle_3rdparty)

add_subdirectory(IceT)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/../vistle/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/../vistle/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/../vistle/lib")

set(COVISE_BUILD TRUE)
set(DESKVOX_USE_CUDA TRUE)
set(DESKVOX_BUILD_VOX_DESK FALSE)
add_subdirectory(deskvox)
