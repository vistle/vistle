set(LIBSIM_SOURCES inSituModule.cpp inSituReader.cpp)

set(LIBSIM_HEADER inSituModule.h inSituReader.h export.h)

vistle_add_library(vistle_insitu_module EXPORT ${LIBSIM_SOURCES} ${LIBSIM_HEADER})
vistle_target_link_libraries(vistle_insitu_module PRIVATE vistle_module vistle_insitu_core PUBLIC vistle_insitu_message)

vistle_install_docs(README.md)
