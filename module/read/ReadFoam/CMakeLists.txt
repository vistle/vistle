if(APPLE)
    # find keg-only libarchive from homebrew
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        set(LibArchive_ROOT /opt/homebrew/opt/libarchive)
    else()
        set(LibArchive_ROOT /usr/local/opt/libarchive)
    endif()
endif()
vistle_find_package(LibArchive)

vistle_find_package(LIBZIP)

vistle_find_package(ZLIB)
if(NOT Boost_FOUND)
    return()
endif()

if(LibArchive_FOUND)
    include_directories(SYSTEM ${LibArchive_INCLUDE_DIRS})
    try_compile(
        LIBARCHIVE_HAVE_READ_CURRENT_POSITION ${CMAKE_BINARY_DIR}/tmp
        ${CMAKE_CURRENT_SOURCE_DIR}/test_archive_read_current_position.cpp
        LINK_LIBRARIES ${LibArchive_LIBRARIES}
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${LibArchive_INCLUDE_DIRS}")
    add_definitions(-DHAVE_LIBARCHIVE)
    if(LIBARCHIVE_HAVE_READ_CURRENT_POSITION)
        add_definitions(-DHAVE_LIBARCHIVE_READ_CURRENT_POSITION)
    else()
        message("ReadFoam: no archive_read_current_position call, resorting to slow path")
    endif()
endif()

if(LIBZIP_FOUND)
    add_definitions(-DHAVE_LIBZIP)
    include_directories(SYSTEM ${LIBZIP_INCLUDE_DIRS})
endif()

add_module(
    ReadFoam
    "read OpenFOAM data"
    ReadFOAM.cpp
    foamtoolbox.cpp
    foamtoolbox.h
    archivemodel.cpp
    archivemodel.h)

target_link_libraries(ReadFoam ${ZLIB_LIBRARIES})
if(Boost_ZLIB_FOUND)
    target_link_libraries(ReadFoam ${Boost_ZLIB_LIBRARIES})
endif()

if(LibArchive_FOUND)
    target_link_libraries(ReadFoam ${LibArchive_LIBRARIES})
endif()

if(LIBZIP_FOUND)
    target_link_libraries(ReadFoam ${LIBZIP_LIBRARIES})
endif()

if(ZLIB_FOUND)
    target_link_libraries(ReadFoam ${ZLIB_LIBRARIES})
endif()

#add_executable(foambounds bounds.cpp foamtoolbox.cpp)
#target_link_libraries(foambounds Boost::boost ${ZLIB_LIBRARIES})
