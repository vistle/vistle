 #pragma once
 #if defined UNCOPYABLE_CLASS
 #undef UNCOPYABLE_CLASS
 #endif
 #if __cplusplus >= 201103L || (defined _MSC_VER && __cplusplus > 199711L)
 #define UNCOPYABLE_CLASS(CLASS_NAME) \
 CLASS_NAME(CLASS_NAME const&) = delete;\
 CLASS_NAME& operator=(CLASS_NAME const&) = delete
 #else
 #define UNCOPYABLE_CLASS(CLASS_NAME) \
 private:\
 CLASS_NAME(CLASS_NAME const&);\
 CLASS_NAME& operator=(CLASS_NAME const&)
 #endif
 #if defined MSWIN
 #define DECLARE_IMPL() \
   \
 __pragma(warning (push)) \
 __pragma(warning (disable:4251)) \
 struct Impl; \
 std::unique_ptr<Impl> const m_impl; \
 __pragma(warning (pop))
 #else
 #define DECLARE_IMPL() \
 struct Impl; \
 std::unique_ptr<Impl> const m_impl;
 #endif
