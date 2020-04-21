set(LIBSIM_SOURCES
  InSituMessage.cpp
  ShmMessage.cpp
  SyncShmIDs.cpp
  TcpMessage.cpp
)

set(LIBSIM_HEADER
  export.h
  InSituMessage.h
  ShmMessage.h
  SyncShmIDs.h
  TcpMessage.h
)

vistle_add_library(vistle_insitu_message ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
target_link_libraries(vistle_insitu_message
		PRIVATE vistle_util
		PRIVATE vistle_core
		vistle_boost_mpi
		PRIVATE ${Boost_LIBRARIES}
		PRIVATE ${MPI_C_LIBRARIES}
		)