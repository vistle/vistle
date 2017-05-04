#ifndef CORE_ASSERT_H
#define CORE_ASSERT_H

#ifndef NDEBUG
#include <util/exception.h>
#include <util/tools.h>
#include <iostream>
#include <sstream>
#endif

#ifdef NDEBUG
#define vassert(true_expr)
#else
#define vassert(true_expr) \
   if (!(true_expr)) { \
      std::stringstream str; \
      str << __FILE__ << ":" << __LINE__ << ": assertion failure: " << #true_expr; \
      std::cerr << str.str() << std::endl << std::flush; \
      std::cerr << vistle::backtrace() << std::endl; \
      vistle::attach_debugger(); \
      throw(vistle::except::assertion_failure(str.str())); \
   }
#endif

#endif
