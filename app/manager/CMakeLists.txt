# Vistle cleaner
vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle Boost::system MPI::MPI_C vistle_core vistle_util_mpi ${BOOST_MPI})

# Vistle manager
set(SOURCES vistle_manager.cpp)

vistle_add_executable(vistle_manager ${SOURCES})

target_link_libraries(
    vistle_manager
    ${BOOST_MPI}
    Boost::system
    MPI::MPI_C
    vistle_control
    vistle_util
    vistle_util_mpi
    vistle_core
    Threads::Threads)

target_link_libraries(vistle_manager vistle_clustermanager)

if(NOT VISTLE_MULTI_PROCESS)
    if(NOT VISTLE_MODULES_SHARED)
        use_openmp()
        foreach(m ${ALL_MODULES})
            if(APPLE)
                target_link_libraries(vistle_manager -Wl,-force_load ${m})
            else()
                target_link_libraries(vistle_manager -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
            endif()
            #target_link_libraries(vistle_clustermanager $<TARGET_OBJECTS:${m}>)
        endforeach()
        target_link_libraries(vistle_manager vistle_module_descriptions)
    endif()
endif()
