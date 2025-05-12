#ifndef VISTLE_CORE_VALIDATE_H
#define VISTLE_CORE_VALIDATE_H


#include <viskores/Types.h>
#include <cstring>


#define VALIDATE_2(condition, msg) \
    if (!(condition)) { \
        os << "CONSISTENCY ERROR in " << __func__ << " at " << __FILE__ << ":" << __LINE__ << ": failing " \
           << #condition << " ***"; \
        if (!std::string(msg).empty()) \
            os << " " << msg << " ***" << std::endl; \
        os << std::endl; \
        print(os); \
        std::stringstream str; \
        str << "CONSISTENCY ERROR in " << __func__ << " at " << __FILE__ << ":" << __LINE__ << ": failing " \
            << #condition << " ***"; \
        throw(vistle::except::consistency_error(str.str())); \
        return false; \
    }
#define VALIDATE_1(condition) VALIDATE_2(condition, "")
#define VALIDATE_X(x, A, B, FUNC, ...) FUNC

//! use in validate(...)
#define VALIDATE(...) \
    VALIDATE_X(, ##__VA_ARGS__, VALIDATE_2(__VA_ARGS__), VALIDATE_1(__VA_ARGS__), VALIDATE_0(__VA_ARGS__))

#define VALIDATE_SUB(sub) \
    if (sub) { \
        VALIDATE(sub->check(os, quick)); \
    }

#define VALIDATE_SUBSIZE(sub, size) \
    if (sub) { \
        VALIDATE(sub->getSize() == size); \
    }

#define VALIDATE_INDEX(idx) \
    VALIDATE(idx < size_t(InvalidIndex)); \
    VALIDATE(idx <= size_t(std::numeric_limits<viskores::Id>::max())); \
    /* VALIDATE(idx >= 0); */

#define VALIDATE_ENUM(val) VALIDATE(strncmp(toString(val), "[Unknown", 8) != 0)

#define VALIDATE_ARRAY(arr, cond) \
    { \
        bool valid = true; \
        for (size_t i = 0; i < arr.size(); ++i) { \
            if (!(cond)) { \
                valid = false; \
                os << #arr << "[" << i << "]=" << (arr)[i] << " violates " << #cond << std::endl; \
            } \
        } \
        VALIDATE(valid, #cond) \
    }

#define VALIDATE_ARRAY_P(ptr, cond) \
    if (ptr) { \
        auto &arr = *(ptr); \
        bool valid = true; \
        for (size_t i = 0; i < arr.size(); ++i) { \
            if (!(cond)) { \
                valid = false; \
                os << #ptr << "[" << i << "]=" << arr[i] << " violates " << #cond << std::endl; \
            } \
        } \
        VALIDATE(valid, #cond) \
    }

#if 0
#define VALIDATE_GEQ(ptr, low) \
    if (ptr) { \
        auto &arr = *(ptr); \
        bool valid = true; \
        for (size_t i = 0; i < arr.size(); ++i) { \
            if (!(arr[i] >= low)) { \
                valid = false; \
                os << #ptr << "[" << i << "]=" << arr[i] << " < " << low << std::endl; \
            } \
        } \
    }
#else
#define VALIDATE_GEQ_P(ptr, low) VALIDATE_ARRAY_P(ptr, arr[i] >= low)
#endif

#define VALIDATE_RANGE(arr, low, up) \
    VALIDATE(low <= up); \
    VALIDATE_ARRAY(arr, (arr[i] >= low && arr[i] <= up))

#define VALIDATE_RANGE_P(ptr, low, up) \
    VALIDATE(low <= up); \
    VALIDATE_ARRAY_P(ptr, (arr[i] >= low && arr[i] <= up))

#define VALIDATE_MONOTONIC(arr) \
    { \
        bool valid = true; \
        for (size_t i = 1; i < arr.size(); ++i) { \
            if (arr[i - 1] > arr[i]) { \
                valid = false; \
                os << #arr << "[" << i - 1 << "]=" << (arr)[i - 1] << " >  successor=" << (arr)[i] << " not monotonic" \
                   << std::endl; \
            } \
        } \
        VALIDATE(valid, #arr " monotonic ") \
    }


#define VALIDATE_MONOTONIC_P(ptr) \
    if (ptr) { \
        auto &arr = *(ptr); \
        bool valid = true; \
        for (size_t i = 1; i < arr.size(); ++i) { \
            if (arr[i - 1] > arr[i]) { \
                valid = false; \
                os << #ptr << "[" << i - 1 << "]=" << (arr)[i - 1] << " >  successor=" << (arr)[i] << " not monotonic" \
                   << std::endl; \
            } \
        } \
        VALIDATE(valid, #ptr " monotonic ") \
    }

#define CHECK_OVERFLOW(expr) \
    do { \
        if ((expr) >= size_t(InvalidIndex)) { \
            throw vistle::except::index_overflow(#expr " = " + std::to_string(expr) + \
                                                 ", recompile with 64 bit indices"); \
        } \
        if ((expr) > size_t(std::numeric_limits<viskores::Id>::max())) { \
            throw vistle::except::index_overflow(#expr " = " + std::to_string(expr) + \
                                                 ", recompile with 64 bit indices"); \
        } \
    } while (false)


#endif
