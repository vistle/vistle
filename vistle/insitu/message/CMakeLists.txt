set(LIBSIM_SOURCES
  InSituMessage.cpp
)

set(LIBSIM_HEADER
  InSituMessage.h
  export.h
)

vistle_add_library(vistle_insitu_message ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
target_link_libraries(vistle_insitu_message
		PRIVATE vistle_util
		PRIVATE vistle_core
		vistle_boost_mpi
		PRIVATE ${Boost_LIBRARIES}
		PRIVATE ${MPI_C_LIBRARIES}
		)