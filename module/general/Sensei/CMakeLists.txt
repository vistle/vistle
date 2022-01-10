add_module(SenseiController "acquire data from SENSEI instrumented simulations" SenseiModule.h SenseiModule.cpp)
target_link_libraries(
    SenseiController
    vistle_module
    vistle_util
    vistle_insitu_message
    vistle_insitu_core
    vistle_insitu_module
    Boost::boost)
