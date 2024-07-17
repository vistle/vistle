#pragma once
/// Error handling utilities

/// Fatal error
#define MAIA_FATAL_ERROR(msg)                                            \
  std::cerr << MString("FATAL ERROR: \"") + msg + MString("\" | At: ") + AT_ << std::endl; \
  throw std::logic_error(MString("FATAL ERROR: \"") + msg + MString("\" | At: ") + AT_)

/// Asserts that index i is in range [lower, upper)
#define MAIA_ASSERT(cnd, msg)                                            \
  if (!(cnd)) {                                                         \
    MAIA_FATAL_ERROR(msg);                                               \
  }

/// Asserts that index i is in range [lower, upper)
#define MAIA_ASSERT_BOUNDS(i, lower, upper)                              \
  MAIA_ASSERT(i >= lower || i < upper, "index " + to_string(i)       \
             + " out-of-bounds [" + to_string(lower) + ", "         \
             + to_string(upper) + ")")

/// Asserts that index i is in range [lower, upper]
#define MAIA_ASSERT_RANGE(i, lower, upper)                        \
  MAIA_ASSERT(i >= lower && i <= upper, "index " + to_string(i) \
             + " out-of-bounds [" + to_string(lower) + ", "   \
             + to_string(upper) + "]")


/// Swallows inflight exceptions and returns RET_VAL instead
#define MAIA_SWALLOW_ERRORS(RET_VAL)                             \
  catch (const std::exception& e) {                             \
    vtkErrorMacro("Exception thrown: " << e.what());            \
    std::cerr << "Exception thrown: " << e.what() << std::endl; \
    return RET_VAL;                                             \
  }
