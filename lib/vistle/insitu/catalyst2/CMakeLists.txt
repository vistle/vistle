# When implementing the Catalyst API, as against using
# it to invoke Catalyst, one must use the component
# ``SDK`` in ``find_package`` call. This ensures that all necessary
# libraries etc. are available.
find_package(catalyst
             REQUIRED
             COMPONENTS SDK)

# use this function call to create a Catalyst API implementation.
catalyst_implementation(
  TARGET  catalyst-vistle
  NAME    vistle
  SOURCES vistleCatalystImpl.cpp conduitToVistle.cpp conduitToVistle.h)

target_link_libraries(catalyst-vistle PRIVATE vistle_insitu_adapter)
target_compile_definitions(catalyst-vistle PRIVATE VISTLE_ROOT="${CMAKE_BINARY_DIR}" VISTLE_BUILD_TYPE="")

