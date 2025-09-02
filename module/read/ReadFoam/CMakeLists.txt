vistle_find_package(LibArchive)
vistle_find_package(LIBZIP)
vistle_find_package(ZLIB)

if(NOT Boost_FOUND)
    return()
endif()

if(LibArchive_FOUND)
    include_directories(SYSTEM ${LibArchive_INCLUDE_DIRS})
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

target_link_libraries(ReadFoam PRIVATE ${ZLIB_LIBRARIES})
if(Boost_ZLIB_FOUND)
    target_link_libraries(ReadFoam PRIVATE ${Boost_ZLIB_LIBRARIES})
endif()

if(LibArchive_FOUND)
    target_link_libraries(ReadFoam PRIVATE ${LibArchive_LIBRARIES})
endif()

if(LIBZIP_FOUND)
    target_link_libraries(ReadFoam PRIVATE ${LIBZIP_LIBRARIES})
endif()

if(ZLIB_FOUND)
    target_link_libraries(ReadFoam PRIVATE ${ZLIB_LIBRARIES})
endif()

#add_executable(foambounds bounds.cpp foamtoolbox.cpp)
#target_link_libraries(foambounds Boost::boost ${ZLIB_LIBRARIES})
