set(SENSEI_SOURCES
  metaData.cpp
  objectRetriever.cpp
  sensei.cpp
)

set(SENSEI_HEADER
  exeption.h
  export.h
  intOption.h
  metaData.h
  objectRetriever.h
  sensei.h
)


<<<<<<< HEAD
=======
set(SENSEI_HEADER exception.h export.h intOption.h metaData.h objectRetriever.h sensei.h)
>>>>>>> 6acc8388b (typos)

vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
vistle_export_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
vistle_target_link_libraries(vistle_sensei
		PRIVATE Boost::boost
    PRIVATE Boost::serialization
		PRIVATE vistle_boost_mpi
    PRIVATE vistle_core
		PRIVATE vistle_util
		PRIVATE vistle_util_mpi
    PRIVATE Threads::Threads
		PUBLIC vistle_insitu_core
		PUBLIC vistle_insitu_message
		PUBLIC	MPI::MPI_C
		)
if(NOT VISTLE_MULTI_PROCESS)
vistle_target_link_libraries(vistle_sensei 
    PRIVATE vistle_clustermanager)
endif()
target_compile_features(vistle_sensei PUBLIC cxx_std_14)
