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

if (VISTLE_SHM_PER_RANK)
   add_definitions(-DSHMPERRANK)
endif()

vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
target_link_libraries(vistle_sensei
		PRIVATE vistle_module
		PRIVATE vistle_util
		PRIVATE vistle_control
        PRIVATE vistle_core
        PRIVATE vistle_boost_mpi
		PRIVATE vistle_insitu_message
		PUBLIC vistle_insitu_core
		PRIVATE ${Boost_LIBRARIES}
		PRIVATE ${MPI_C_LIBRARIES}
		)