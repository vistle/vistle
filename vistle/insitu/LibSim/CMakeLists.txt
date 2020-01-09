#library to send a initialize message to a LibSim simulation to connect that simulation to the ConnectLibSim module
vistle_add_library(libsim_connect EstablishConnection.h EstablishConnectionExports.h EstablishConnection.cpp)
target_link_libraries(libsim_connect
		PRIVATE vistle_util
		PRIVATE ${Boost_LIBRARIES}
		)


