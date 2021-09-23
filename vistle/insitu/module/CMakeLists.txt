set(LIBSIM_SOURCES inSituReader.cpp)

set(LIBSIM_HEADER inSituReader.h export.h)

vistle_add_library(vistle_insitu_module ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
vistle_export_library(vistle_insitu_module ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
vistle_target_link_libraries(vistle_insitu_module PRIVATE vistle_module PUBLIC vistle_insitu_message)
vistle_install_docs(README.md)
