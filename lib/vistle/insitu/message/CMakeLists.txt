set(SOURCES InSituMessage.cpp ShmMessage.cpp SyncShmIDs.cpp TcpMessage.cpp addObjectMsq.cpp moduleInfo.cpp)

set(HEADER
    InSituMessage.h
    ShmMessage.h
    SyncShmIDs.h
    TcpMessage.h
    addObjectMsq.h
    export.h
    moduleInfo.h
    sharedOption.h)

vistle_add_library(vistle_insitu_message ${SOURCES} ${HEADER})
vistle_export_library(vistle_insitu_message ${SOURCES} ${HEADER})

vistle_target_link_libraries(
    vistle_insitu_message
    PRIVATE
    vistle_core
    PRIVATE
    vistle_util
    PRIVATE
    ${BOOST_MPI}
    PRIVATE
    vistle_insitu_core
    PRIVATE
    Boost::boost
    PRIVATE
    MPI::MPI_C
    PRIVATE
    Threads::Threads)
vistle_install_docs(README.md)
