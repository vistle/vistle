set(SENSEI_SOURCES metaData.cpp objectRetriever.cpp sensei.cpp)
set(SENSEI_HEADER exception.h export.h metaData.h objectRetriever.h sensei.h)

vistle_add_library(vistle_sensei EXPORT ${SENSEI_SOURCES} ${SENSEI_HEADER})

vistle_target_link_libraries(
    vistle_sensei
    PRIVATE
    Boost::boost
    PRIVATE
    Boost::serialization
    PUBLIC
    ${BOOST_MPI}
    PUBLIC
    vistle_core
    PRIVATE
    vistle_util
    PRIVATE
    vistle_util_mpi
    PRIVATE
    Threads::Threads
    PUBLIC
    vistle_insitu_core
    PUBLIC
    vistle_insitu_message
    PUBLIC
    MPI::MPI_C)
if(NOT VISTLE_MULTI_PROCESS)
    vistle_target_link_libraries(vistle_sensei PRIVATE vistle_clustermanager)
    target_compile_definitions(vistle_sensei INTERFACE VISTLE_SINGLE_PROCESS)
endif()
target_compile_features(vistle_sensei PUBLIC cxx_std_17)
