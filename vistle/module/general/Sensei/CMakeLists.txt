# Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
# Project developers.  See the top-level LICENSE file for dates and other
# details.  No copyright assignment is required to contribute to VisIt.

#****************************************************************************
# Modifications:
#
#****************************************************************************

#SenseiModule
add_module(SenseiController SenseiModule.h SenseiModule.cpp)
target_link_libraries(SenseiController
	vistle_module
	vistle_util
	vistle_insitu_message
	vistle_insitu_module
	Boost::boost
	)


