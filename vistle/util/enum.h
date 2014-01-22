#ifndef ENUM_H
#define ENUM_H

// cf. http://stackoverflow.com/questions/5093460/how-to-convert-an-enum-type-variable-to-a-string
// define an enum like this:
//    DEFINE_ENUM_WITH_STRING_CONVERSIONS(OS_type, (Linux)(Apple)(Windows))

#include <boost/preprocessor.hpp>

#ifdef ENUMS_FOR_PYTHON
#include <boost/python/enum.hpp>
#endif

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)   \
   case elem : return BOOST_PP_STRINGIZE(elem);

#define X_DEFINE_ENUM_FOR_PYTHON_VALUE(r, data, elem)                        \
   .value(BOOST_PP_STRINGIZE(elem), elem)

#ifdef ENUMS_FOR_PYTHON
#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)               \
enum name {                                                                  \
   BOOST_PP_SEQ_ENUM(enumerators)                                            \
};                                                                           \
                                                                             \
static inline const char *toString(name v) {                                 \
   switch (v) {                                                              \
      BOOST_PP_SEQ_FOR_EACH(                                                 \
            X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,             \
            name,                                                            \
            enumerators                                                      \
      )                                                                      \
      default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";              \
   }                                                                         \
} \
\
static inline void enumForPython_##name(const char *pythonName) { \
   boost::python::enum_<name>(pythonName) \
      BOOST_PP_SEQ_FOR_EACH(                                                 \
            X_DEFINE_ENUM_FOR_PYTHON_VALUE, \
            name, \
            enumerators \
      ) \
   ; \
}

#define V_ENUM_OUTPUT_OP(name, scope) \
   inline std::ostream &operator<<(std::ostream &s, scope::name v) { \
      s << scope::toString(v) << " (" << (int)v << ")"; \
      return s; \
}
#else
#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)               \
enum name {                                                                  \
   BOOST_PP_SEQ_ENUM(enumerators)                                            \
};                                                                           \
                                                                             \
static inline const char *toString(name v) {                                 \
   switch (v) {                                                              \
      BOOST_PP_SEQ_FOR_EACH(                                                 \
            X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,             \
            name,                                                            \
            enumerators                                                      \
      )                                                                      \
      default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";              \
   }                                                                         \
}

#define V_ENUM_OUTPUT_OP(name, scope) \
   inline std::ostream &operator<<(std::ostream &s, scope::name v) { \
      s << scope::toString(v) << " (" << (int)v << ")"; \
      return s; \
}
#endif

#endif
