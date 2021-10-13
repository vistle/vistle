add_module(Dropbear "start Dropbear SSH server on compute nodes" Dropbear.cpp)

target_link_libraries(Dropbear ${CMAKE_DL_LIBS})
