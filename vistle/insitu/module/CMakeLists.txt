set(LIBSIM_SOURCES
  inSituReader.cpp
)

set(LIBSIM_HEADER
  inSituReader.h
  export.h
)
message("calling vistle_add_library")
vistle_add_library(vistle_insitu_module ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
vistle_export_library(vistle_insitu_module ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
vistle_link_libraries(vistle_insitu_module
		PRIVATE vistle_module
		PUBLIC vistle_insitu_message
		)
vistle_install_docs(README.md)