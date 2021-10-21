#ifndef STOPWATCH_H
#define STOPWATCH_H

#include "export.h"

#include <string>
#include <chrono>

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

private:
    std::string m_description;
    clock_type::time_point m_start;
};

} // namespace vistle

#endif
