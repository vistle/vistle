#ifndef CORE_ASSERT_H
#define CORE_ASSERT_H

#ifndef NDEBUG
#include "exception.h"
#include <util/tools.h>
#include <iostream>
#include <sstream>
#endif

#ifdef NDEBUG
#define vassert(true_expr)
#else
#define vassert(true_expr) \
   if (!(true_expr)) { \
      std::cerr << "assertion failure: " << __FILE__ << ":" << __LINE__ << ": " << #true_expr << std::endl; \
      std::stringstream str; \
      str << __FILE__ << ":" << __LINE__ << ": assertion failure: " << #true_expr; \
      vistle::attach_debugger(); \
      throw(vistle::except::assertion_failure(str.str())); \
   }
#endif

#ifdef assert
#undef assert
#endif

#define assert(x) vassert(x)

#endif
