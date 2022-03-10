set(SOURCES exception.cpp slowMpi.cpp attachVistleShm.cpp)
set(HEADER
    callFunctionWithVoidToTypeCast.h
    dataType.h
    exception.h
    export.h
    slowMpi.h
    transformArray.h
    attachVistleShm.h)

vistle_add_library(vistle_insitu_core EXPORT ${SOURCES} ${HEADER})

vistle_target_link_libraries(
    vistle_insitu_core
    PRIVATE
    Boost::boost
    Boost::serialization
    MPI::MPI_C
    ${BOOST_MPI}
    vistle_util
    vistle_util_mpi
    PUBLIC
    vistle_core)
