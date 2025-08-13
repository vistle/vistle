# When implementing the Catalyst API, as against using
# it to invoke Catalyst, one must use the component
# ``SDK`` in ``find_package`` call. This ensures that all necessary
# libraries etc. are available.

vistle_install_docs(README.md) # install unconditionally

vistle_find_package(catalyst 2 COMPONENTS SDK)

if(NOT catalyst_FOUND)
    return()
endif()

get_target_property(EXTERNAL_CONDUIT catalyst::catalyst CATALYST_WITH_EXTERNAL_CONDUIT)
if(EXTERNAL_CONDUIT)
    vistle_find_package(conduit)
    if(NOT conduit_FOUND)
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
    vistleCatalystImpl.cpp
    conduitToVistle.cpp
    conduitToVistle.h)

target_link_libraries(catalyst-vistle PRIVATE vistle_insitu_adapter)
if(EXTERNAL_CONDUIT)
    target_link_libraries(catalyst-vistle PRIVATE conduit::conduit)
endif()

target_compile_definitions(catalyst-vistle PRIVATE VISTLE_ROOT="${CMAKE_BINARY_DIR}" VISTLE_BUILD_TYPE="")
