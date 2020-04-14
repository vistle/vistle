set(SENSEI_SOURCES
  dataAdaptor.cpp
  sensei.cpp
)

set(SENSEI_HEADER
  export.h  
  dataAdaptor.h  
  sensei.h

)

vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
target_link_libraries(vistle_sensei
		vistle_module
		vistle_util
		vistle_control
        vistle_core
        vistle_boost_mpi
		vistle_insitu_message
		${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
		)