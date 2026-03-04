#ifndef VISTLE_UTIL_PROFILE_H
#define VISTLE_UTIL_PROFILE_H

#if defined(VISTLE_PROFILE_NVTX)
#include <nvtx3/nvtx3.hpp>
#define PROF_FUNC() NVTX3_FUNC_RANGE()
#define PROF_SCOPE(name) nvtx3::scoped_range vistle_profile_range(name)
#define PROF_MARK(name) nvtx3::mark(name)

#elif defined(VISTLE_PROFILE_CHROME)
#include <Profiler.hpp>
#define PROF_FUNC() PROFILE_FUNCTION()
#define PROF_SCOPE(name) PROFILE_SCOPE(name)
#define PROF_MARK(name) \
    { \
        PROFILE_SCOPE(name); \
    }

#elif defined(VISTLE_PROFILE_ROCTX)
#include <rocprofiler-sdk-roctx/roctx.h>
class roctxScopedRange {
private:
    int id;

public:
    explicit roctxScopedRange(const char *name) { id = roctxRangeStart(name); }

    ~roctxScopedRange() { roctxRangeStop(id); }
};

#define PROF_FUNC() roctxScopedRange vistle_profile_function(__FUNCTION__)
#define PROF_SCOPE(name) roctxScopedRange vistle_profile_range(name)
#define PROF_MARK(name) roctxMark(name)

#else
#define PROF_FUNC()
#define PROF_SCOPE(name)
#define PROF_MARK(name)
#endif

#endif
