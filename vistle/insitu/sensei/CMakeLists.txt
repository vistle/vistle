set(SENSEI_SOURCES
  rectilinerGrid.cpp
  sensei.cpp
  structuredGrid.cpp
  unstructuredGrid.cpp
)

set(SENSEI_HEADER
  exeption.h
  export.h  
  metaData.h
  rectilinerGrid.h
  sensei.h
  structuredGrid.h
  unstructuredGrid.h
)

vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
target_link_libraries(vistle_sensei
		vistle_module
		vistle_util
		vistle_control
        vistle_core
        vistle_boost_mpi
		vistle_insitu_message
		vistle_insitu_core
		${Boost_LIBRARIES}
		${MPI_C_LIBRARIES}
		)