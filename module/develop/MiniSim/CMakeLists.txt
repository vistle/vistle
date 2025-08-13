set(HEADER MiniSim.h minisim/analysis.h minisim/Block.h minisim/minisim.h minisim/Oscillator.h minisim/Particles.h)
set(SOURCES MiniSim.cpp minisim/analysis.cpp minisim/Block.cpp minisim/minisim.cpp minisim/Oscillator.cpp minisim/Particles.cpp)

# find_path(DIY_INCLUDE_DIR NAMES diy/critical-resource.hpp) #critical-resource.hpp is an arbitrary file in the diy include dir
# message("DIY_INCLUDE_DIR = ${DIY_INCLUDE_DIR}")
if(DIY_INCLUDE_DIR)
    # message("building minisim")
    add_module(MiniSim "small simulation to compare performance with other in situ interfaces" ${HEADER} ${SOURCES})
    target_include_directories(MiniSim PRIVATE ${DIY_INCLUDE_DIR})
    target_link_libraries(
        MiniSim
        Boost::boost
        vistle_insitu_core
        vistle_insitu_message
        vistle_insitu_module
        vistle_libsim_connect
        vistle_module
        vistle_insitu_adapter
        vistle_util)

    if(WIN32)
        # On Windows, we need to set the character set to UTF-8
        target_compile_options(MiniSim PRIVATE /utf-8)
    else()
        target_compile_options(MiniSim PRIVATE -Wno-sign-compare)
    endif()

    target_compile_definitions(MiniSim PRIVATE VISTLE_ROOT="${CMAKE_BINARY_DIR}" VISTLE_BUILD_TYPE="")
endif()
