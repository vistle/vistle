set(SOURCES
  array.cpp
  dataAdaptorToVistle.cpp
  exeption.cpp
  metaData.cpp
  )
set(HEADER
  array.h
  callFunctionWithVoidToTypeCast.h
  dataAdaptorToVistle.h
  dataType.h
  exeption.h
  export.h
  metaData.h
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