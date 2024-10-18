set(ADAPTER_SOURCES metaData.cpp objectRetriever.cpp adapter.cpp)
set(ADAPTER_HEADER exception.h export.h metaData.h objectRetriever.h adapter.h)

vistle_add_library(vistle_insitu_adapter EXPORT ${ADAPTER_SOURCES} ${ADAPTER_HEADER})

vistle_target_link_libraries(
    vistle_insitu_adapter
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
    MPI::MPI_CXX)
if(NOT VISTLE_MULTI_PROCESS)
    vistle_target_link_libraries(vistle_insitu_adapter PRIVATE vistle_clustermanager)
    target_compile_definitions(vistle_insitu_adapter INTERFACE VISTLE_SINGLE_PROCESS)
endif()
target_compile_features(vistle_insitu_adapter PUBLIC cxx_std_17)
