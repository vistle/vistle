add_module(Cache "cache input objects" Cache.cpp vistle_file.cpp vistle_file.h)

add_executable(vistle_ls vistle_ls.cpp vistle_file.cpp vistle_file.h)
target_link_libraries(vistle_ls PRIVATE vistle_util vistle_core)
