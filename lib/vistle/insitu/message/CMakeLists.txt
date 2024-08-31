set(SOURCES
    InSituMessage.cpp
    ShmMessage.cpp
    SyncShmIDs.cpp
    TcpMessage.cpp
    addObjectMsq.cpp
    moduleInfo.cpp
    sharedParam.cpp)

set(HEADER
    InSituMessage.h
    MessageHandler.h
    ShmMessage.h
    SyncShmIDs.h
    TcpMessage.h
    addObjectMsq.h
    export.h
    moduleInfo.h
    sharedParam.h)

vistle_add_library(vistle_insitu_message EXPORT ${SOURCES} ${HEADER})

vistle_target_link_libraries(
    vistle_insitu_message
    PRIVATE
    ${BOOST_MPI}
    Boost::boost
    MPI::MPI_CXX
    Threads::Threads
    vistle_insitu_core
    vistle_util
    PUBLIC
    vistle_core)

vistle_install_docs(README.md)
