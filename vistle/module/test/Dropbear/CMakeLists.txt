add_module(Dropbear "Dropbear SSH server" Dropbear.cpp)

target_link_libraries(Dropbear ${CMAKE_DL_LIBS})
