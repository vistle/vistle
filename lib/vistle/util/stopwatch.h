#ifndef VISTLE_UTIL_STOPWATCH_H
#define VISTLE_UTIL_STOPWATCH_H

#include "export.h"

#include <string>
#include <chrono>
#include <memory>

namespace boost {
namespace timer {
class cpu_timer;
class auto_cpu_timer;
} // namespace timer
} // namespace boost

namespace vistle {

namespace chrono = std::chrono;

class V_UTILEXPORT Clock {
public:
    typedef chrono::high_resolution_clock clock_type;

    static double time();
};

class V_UTILEXPORT StopWatch {
public:
    typedef chrono::high_resolution_clock clock_type;

    StopWatch(const char *description);
    ~StopWatch();
    void reset();
    void stop();
    void resume();
    void report();

private:
    std::string m_description;
    std::string m_format;
    std::unique_ptr<boost::timer::auto_cpu_timer> m_timer;
};

} // namespace vistle

#endif
