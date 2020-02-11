# Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
# Project developers.  See the top-level LICENSE file for dates and other
# details.  No copyright assignment is required to contribute to VisIt.

#****************************************************************************
# Modifications:
#
#****************************************************************************

#LibSimModule
add_module(LibSimModule LibSimModule.h LibSimModule.cpp)
target_link_libraries(LibSimModule
	vistle_module
	vistle_util
	vistle_insitu_message
	vistle_insitu_module
	vistle_libsim_connect
	${Boost_LIBRARIES}
	)



