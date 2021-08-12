

set(HEADER
MiniSim.h
minisim/analysis.h
minisim/Block.h
minisim/minisim.h
minisim/Oscillator.h
minisim/Particles.h
)

set(SOURCES
MiniSim.cpp
minisim/analysis.cpp
minisim/Block.cpp
minisim/minisim.cpp
minisim/Oscillator.cpp
minisim/Particles.cpp
)
find_path(DIY_INCLUDE_DIR Names diy/critical-resource.hpp) #critical-resource.hpp is an arbitrary file in the diy inclue dir
message("DIY_INCLUDE_DIR = "${DIY_INCLUDE_DIR})
if(EXISTS ${DIY_INCLUDE_DIR})
    message("building minisim")
	add_module(MiniSim ${HEADER} ${SOURCES})
    target_include_directories(MiniSim PRIVATE ${DIY_INCLUDE_DIR})
	target_link_libraries(MiniSim
		Boost::boost
		vistle_insitu_core
		vistle_insitu_message
		vistle_insitu_module
		vistle_libsim_connect
		vistle_module
		vistle_util
		)
endif()




