#include "stopwatch.h"

#include <iostream>

namespace vistle {

StopWatch::StopWatch(const char *description)
: m_description(description ? description : "")
, m_start(clock_type::now())
{
}

StopWatch::~StopWatch() {

   const clock_type::time_point stop = clock_type::now();
   const double duration = 1e-9*chrono::duration_cast<chrono::nanoseconds>(stop-m_start).count();
   std::cerr << m_description << ": " << duration << "s" << std::endl;
}

} // namespace vistle
