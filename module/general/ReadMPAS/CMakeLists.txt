vistle_find_package(PNETCDF)
if(NOT PNETCDF_FOUND)
    return()
endif()

set(HEADERS ReadMPAS.h)

set(SOURCES ReadMPAS.cpp)

add_module(ReadMPAS "Read MPAS files" ${SOURCES} ${HEADERS})

target_link_libraries(ReadMPAS ${PNETCDF_LIBRARIES})
target_include_directories(ReadMPAS SYSTEM PRIVATE ${PNETCDF_INCLUDE_DIRS})
