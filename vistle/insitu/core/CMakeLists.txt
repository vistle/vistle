set(SOURCES
  dataAdaptor.cpp
  dataAdaptorToVistle.cpp
  exeption.cpp
  )
set(HEADER
  dataAdaptor.h
  dataAdaptorToVistle.h
  exeption.h
  export.h
  moduleInfo.h
  transformArray.h
  )
vistle_add_library(vistle_insitu_core ${SOURCES} ${HEADER})
vistle_export_library(vistle_insitu_core ${SOURCES} ${HEADER})
vistle_target_link_libraries(vistle_insitu_core 
	PUBLIC vistle_core
	PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE MPI::MPI_C
	)