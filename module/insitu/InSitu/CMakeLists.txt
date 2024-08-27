add_module(InSituModule "acquire data from SENSEI or Catalyst II instrumented simulations" InSituModule.h InSituModule.cpp)
target_link_libraries(
    InSituModule
    vistle_module
    vistle_util
    vistle_insitu_message
    vistle_insitu_core
    vistle_insitu_module
    Boost::boost)
