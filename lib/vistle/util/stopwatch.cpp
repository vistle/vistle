#include "stopwatch.h"

#include <iostream>
#include <boost/timer/timer.hpp>

namespace vistle {

double Clock::time()
{
    const clock_type::time_point now = clock_type::now();
    return 1e-9 * chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch()).count();
}

StopWatch::StopWatch(const char *description)
: m_description(description ? description : "")
, m_format(m_description + ": %ws wall, %us user + %ss sys (%p%)\n")
, m_timer(std::make_unique<boost::timer::auto_cpu_timer>(std::cerr, 3, m_format))
{}

void StopWatch::stop()
{
    m_timer->stop();
}

void StopWatch::resume()
{
    m_timer->resume();
}

void StopWatch::report()
{
    m_timer->report();
}

void StopWatch::reset()
{
    m_timer = std::make_unique<boost::timer::auto_cpu_timer>(std::cerr, 3, m_format);
}


StopWatch::~StopWatch()
{}

} // namespace vistle
