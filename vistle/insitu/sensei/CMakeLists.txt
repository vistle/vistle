set(SENSEI_SOURCES
  callbacks.cpp
  metaData.cpp
  sensei.cpp
)

set(SENSEI_HEADER
  callbacks.h
  exeption.h
  export.h
  metaData.h
  sensei.h
)



vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
vistle_export_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
vistle_target_link_libraries(vistle_sensei
		PRIVATE Boost::boost
        PRIVATE Boost::serialization
		PRIVATE vistle_boost_mpi
        PRIVATE vistle_core
		PRIVATE vistle_util
		PUBLIC vistle_insitu_core
		PUBLIC vistle_insitu_message
		PUBLIC	MPI::MPI_C
		)

target_compile_features(vistle_sensei PUBLIC cxx_std_14)

if (VISTLE_SHM_PER_RANK)
    target_compile_definitions(vistle_sensei 
        PRIVATE -DSHMPERRANK)
endif()