 #if defined(_MSC_VER)
 #pragma warning (pop)
 #if (_MSC_VER > 1600)
 #pragma warning (pop)
 #endif
 #elif defined(__clang__)
 #pragma clang diagnostic pop
 #elif defined(__GNUC__)
 #if ((__GNUC__  >= 4) && (__GNUC_MINOR__ >= 6)) || (__GNUC__ >= 5)
 #pragma GCC diagnostic pop
 #if defined BOOST_GEOMETRY_DISABLE_DEPRECATED_03_WARNING_was_set
 #undef BOOST_GEOMETRY_DISABLE_DEPRECATED_03_WARNING_was_set
 #else
 #undef BOOST_GEOMETRY_DISABLE_DEPRECATED_03_WARNING
 #endif
 #if defined BOOST_MATH_DISABLE_DEPRECATED_03_WARNING_was_set
 #undef BOOST_MATH_DISABLE_DEPRECATED_03_WARNING_was_set
 #else
 #undef BOOST_MATH_DISABLE_DEPRECATED_03_WARNING
 #endif
 #endif
 #endif
