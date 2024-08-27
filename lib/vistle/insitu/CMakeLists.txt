add_subdirectory(core)
add_subdirectory(message) #depends on core
add_subdirectory(module) #depends on core and message

add_subdirectory(libsim)

add_subdirectory(adapter)

