vistle_find_package(HDF5)
if ((NOT HDF5_FOUND))
    message("Skipping because of missing HDF5")
  return()
else()
    include_directories(SYSTEM ${HDF5_INCLUDES})
    set(EXTRA_LIBS ${EXTRA_LIBS} ${HDF5_LIBRARIES} ${HDF5_C++_LIBRARY})
endif()

set(HEADERS
    ReadSeisSol.h
    surfacetools.h
)

set(SOURCES
    ReadSeisSol.cpp
    surfacetools.cpp
)

add_module(ReadSeisSol ${SOURCES} ${HEADERS})

target_link_libraries(ReadSeisSol ${EXTRA_LIBS})

