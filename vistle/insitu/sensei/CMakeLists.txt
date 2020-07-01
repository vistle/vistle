set(SENSEI_SOURCES
  multiGrid.cpp
  rectilinerGrid.cpp
  senseiImpl.cpp
  senseiInterface.cpp
  structuredGrid.cpp
  structuredGridBase.cpp
  unstructuredGrid.cpp
)

set(SENSEI_HEADER
  exeption.h
  export.h
  gridInterface.h
  multiGrid.h
  rectilinerGrid.h
  senseiImpl.h
  senseiInterface.h
  structuredGrid.h
  structuredGridBase.h
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

target_compile_features(vistle_sensei PUBLIC cxx_std_14)

if (VISTLE_SHM_PER_RANK)
    target_compile_definitions(vistle_sensei 
        PRIVATE -DSHMPERRANK)
endif()