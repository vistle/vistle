# When implementing the Catalyst API, as against using
# it to invoke Catalyst, one must use the component
# ``SDK`` in ``find_package`` call. This ensures that all necessary
# libraries etc. are available.

vistle_install_docs(README.md) # install unconditionally

vistle_find_package(catalyst 2 COMPONENTS SDK CONFIG)

if(NOT catalyst_FOUND)
    return()
endif()

get_target_property(EXTERNAL_CONDUIT catalyst::catalyst CATALYST_WITH_EXTERNAL_CONDUIT)
if(EXTERNAL_CONDUIT)
    vistle_find_package(conduit)
    if(NOT conduit_FOUND)
        return()
    endif()
else()
    # paraview-catalyst 2.0.0 on Arch linux brings catalyst, but not conduit.hpp, even though it's included by installed headers
    get_target_property(CONDUIT_INCLUDE_DIRS catalyst::catalyst_headers INTERFACE_INCLUDE_DIRECTORIES)
    find_file(CONDUIT_HPP conduit.hpp PATHS ${CONDUIT_INCLUDE_DIRS})
    if(NOT CONDUIT_HPP)
        message("Did not find `conduit.hpp` in `${CONDUIT_INCLUDE_DIRS}`, but found catalyst 2")
        return()
    endif()
endif()

# use this function call to create a Catalyst API implementation.
catalyst_implementation(
    TARGET
    catalyst-vistle
    NAME
    vistle
    SOURCES
    conduitTopology.cpp
    conduitTopology.h
    conduitToVistle.cpp
    conduitToVistle.h
    vistleCatalystImpl.cpp)

target_link_libraries(catalyst-vistle PRIVATE vistle_insitu_adapter)
if(EXTERNAL_CONDUIT)
    target_link_libraries(catalyst-vistle PRIVATE conduit::conduit)
endif()

target_compile_definitions(catalyst-vistle PRIVATE VISTLE_ROOT="${CMAKE_BINARY_DIR}" VISTLE_BUILD_TYPE="")
