#ifndef VISTLE_UTIL_ENUM_H
#define VISTLE_UTIL_ENUM_H

// cf. http://stackoverflow.com/questions/5093460/how-to-convert-an-enum-type-variable-to-a-string
// define an enum like this:
//    DEFINE_ENUM_WITH_STRING_CONVERSIONS(OS_type, (Linux)(Apple)(Windows))

#include <vector>
#include <string>
#include <boost/preprocessor.hpp>

#ifdef ENUMS_FOR_PYTHON
#include "pybind.h"
#endif

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem) \
    case elem: \
        return BOOST_PP_STRINGIZE(elem);

#define X_DEFINE_ENUM_FOR_PYTHON_VALUE(r, data, elem) .value(BOOST_PP_STRINGIZE(elem), elem)

#define X_DEFINE_ENUM_ADD_VALUE(r, data, elem) values.emplace_back(BOOST_PP_STRINGIZE(elem));

#ifdef ENUMS_FOR_PYTHON
#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators) \
    enum name { BOOST_PP_SEQ_ENUM(enumerators) }; \
\
    static inline const char *toString(name v) \
    { \
        switch (v) { \
            BOOST_PP_SEQ_FOR_EACH(X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE, name, enumerators) \
        default: \
            return "[Unknown " BOOST_PP_STRINGIZE(name) "]"; \
        } \
    } \
\
    static inline std::vector<std::string> valueList(name) \
    { \
        std::vector<std::string> values; \
        BOOST_PP_SEQ_FOR_EACH(X_DEFINE_ENUM_ADD_VALUE, name, enumerators) \
        return values; \
    } \
\
    template<class Scope> \
    static inline void enumForPython_##name(const Scope &scope, const char *pythonName) \
    { \
        namespace py = pybind11; \
        py::enum_<name>(scope, pythonName) BOOST_PP_SEQ_FOR_EACH(X_DEFINE_ENUM_FOR_PYTHON_VALUE, name, enumerators) \
            .export_values(); \
    }
#else
#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators) \
    enum name { BOOST_PP_SEQ_ENUM(enumerators) }; \
\
    static inline const char *toString(name v) \
    { \
        switch (v) { \
            BOOST_PP_SEQ_FOR_EACH(X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE, name, enumerators) \
        default: \
            return "[Unknown " BOOST_PP_STRINGIZE(name) "]"; \
        } \
    } \
\
    static inline std::vector<std::string> valueList(name) \
    { \
        std::vector<std::string> values; \
        BOOST_PP_SEQ_FOR_EACH(X_DEFINE_ENUM_ADD_VALUE, name, enumerators) \
        return values; \
    }
#endif

#define V_ENUM_OUTPUT_OP(name, scope) \
    inline std::ostream &operator<<(std::ostream &s, scope::name v) \
    { \
        s << scope::toString(v) << " (" << (int)v << ")"; \
        return s; \
    }

#define V_ENUM_SET_CHOICES_SCOPE(param, name, scope) setParameterChoices(param, scope::valueList((scope::name)0))

#define V_ENUM_SET_CHOICES(param, name) setParameterChoices(param, valueList((name)0))
#endif
