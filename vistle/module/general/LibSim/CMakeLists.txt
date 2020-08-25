

add_module(LibSimController LibSimModule.h LibSimModule.cpp)
target_link_libraries(LibSimController
	vistle_module
	vistle_util
	vistle_insitu_message
	vistle_insitu_module
	vistle_libsim_connect
	Boost::boost
	)

target_include_directories(LibSimController PRIVATE 
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vistle/insitu/libsim>
$<INSTALL_INTERFACE:include/vistle/insitu/libsim>  
)

if(NOT VISTLE_MULTI_PROCESS)
target_link_libraries(LibSimController
		vistle_libsim_engine_interface
		)
endif()
vistle_install_docs(README.md)

