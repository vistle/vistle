set(SOURCES
  addObjectMsq.cpp
  InSituMessage.cpp
  ShmMessage.cpp
  SyncShmIDs.cpp
  TcpMessage.cpp
)

set(HEADER
  addObjectMsq.h
  InSituMessage.h
  ShmMessage.h
  SyncShmIDs.h
  TcpMessage.h
)

vistle_add_library(vistle_insitu_message ${SOURCES} ${HEADER})
include(GenerateExportHeader)
GENERATE_EXPORT_HEADER(vistle_insitu_message
)
target_include_directories(vistle_insitu_message PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(vistle_insitu_message
		PRIVATE vistle_util
		PRIVATE vistle_insitu_core
		PRIVATE vistle_boost_mpi
		PRIVATE ${Boost_LIBRARIES}
		PRIVATE ${MPI_C_LIBRARIES}
		)