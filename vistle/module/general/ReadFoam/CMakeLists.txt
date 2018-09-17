set(BOOST_COMPONENTS iostreams filesystem system)
if (WIN32)
   set(BOOST_COMPONENTS ${BOOST_COMPONENTS} zlib)
endif()

if (APPLE)
   # find keg-only libarchive from homebrew
   set(LibArchive_ROOT /usr/local/opt/libarchive)
endif()
vistle_find_package(LibArchive)

#vistle_find_package(ZLIB)
vistle_find_package(Boost 1.53 COMPONENTS ${BOOST_COMPONENTS})
if (NOT Boost_FOUND)
    return()
endif()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

if (LibArchive_FOUND)
   include_directories(SYSTEM ${LibArchive_INCLUDE_DIRS})
   add_definitions(-DHAVE_LIBARCHIVE)
endif()

add_module(ReadFoam ReadFOAM.cpp
   foamtoolbox.cpp foamtoolbox.h
   archivemodel.cpp archivemodel.h)

target_link_libraries(ReadFoam
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        ${ZLIB_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)

if (LibArchive_FOUND)
   target_link_libraries(ReadFoam ${LibArchive_LIBRARIES})
endif()

#add_executable(foambounds bounds.cpp foamtoolbox.cpp)
#target_link_libraries(foambounds ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})
