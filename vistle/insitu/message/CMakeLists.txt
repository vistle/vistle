set(SOURCES
  addObjectMsq.cpp
  InSituMessage.cpp
  ShmMessage.cpp
  SyncShmIDs.cpp
  TcpMessage.cpp
)

set(HEADER
  export.h
  addObjectMsq.h
  InSituMessage.h
  ShmMessage.h
  SyncShmIDs.h
  TcpMessage.h
  sharedOption.h
)

vistle_add_library(vistle_insitu_message ${SOURCES} ${HEADER})
vistle_export_library(vistle_insitu_message ${SOURCES} ${HEADER})

vistle_target_link_libraries(vistle_insitu_message
		PRIVATE vistle_core
		PRIVATE vistle_util
		PRIVATE vistle_boost_mpi
		PRIVATE vistle_insitu_core
		PRIVATE Boost::boost
		PRIVATE MPI::MPI_C
        PRIVATE Threads::Threads
		)
vistle_install_docs(README.md)
