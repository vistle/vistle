set(SOURCES exception.cpp slowMpi.cpp)
set(HEADER callFunctionWithVoidToTypeCast.h dataType.h exception.h export.h slowMpi.h transformArray.h)

vistle_add_library(vistle_insitu_core ${SOURCES} ${HEADER})
vistle_export_library(vistle_insitu_core ${SOURCES} ${HEADER})
vistle_target_link_libraries(
    vistle_insitu_core
    PRIVATE
    Boost::boost
    PRIVATE
    Boost::serialization
    PRIVATE
    MPI::MPI_C
    PRIVATE
    ${BOOST_MPI}
    PRIVATE
    vistle_util
    PUBLIC
    vistle_core)
