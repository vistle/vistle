set(LIBSIM_SOURCES
  inSituReader.cpp
)

set(LIBSIM_HEADER
  inSituReader.h
  export.h
)

vistle_add_library(vistle_insitu_module ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
target_link_libraries(vistle_insitu_module
		PRIVATE vistle_module
		)