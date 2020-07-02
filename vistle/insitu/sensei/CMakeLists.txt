set(SENSEI_SOURCES
  rectilinerGrid.cpp
  senseiImpl.cpp
  senseiInterface.cpp
  structuredGrid.cpp
  unstructuredGrid.cpp
)

set(SENSEI_HEADER
  exeption.h
  export.h
  metaData.h
  rectilinerGrid.h
  senseiImpl.h
  senseiInterface.h
  structuredGrid.h
  unstructuredGrid.h
)



vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
vistle_export_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
vistle_target_link_libraries(vistle_sensei
		PRIVATE vistle_util
        PRIVATE vistle_core
		PRIVATE vistle_insitu_message
		PUBLIC vistle_insitu_core
		PRIVATE Boost::boost
        PRIVATE Boost::serialization
		PRIVATE MPI::MPI_C
		)

if (VISTLE_SHM_PER_RANK)
    target_compile_definitions(vistle_sensei 
        PRIVATE -DSHMPERRANK)
endif()