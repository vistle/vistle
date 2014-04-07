#ifndef UTIL_OPENMP_H
#define UTIL_OPENMP_H

#ifdef _OPENMP
#include <omp.h>

#if defined(__GNUC__) && (__GNUC__>4 || __GNUC_MINOR__>=7)
#define HAVE_OMP_3_1
#endif

#ifdef __INTEL_COMPILER
#define HAVE_OMP_3_1
#endif
#endif

template <typename T>
inline T fetch_and_add(volatile T& value, T increment) {

   T old;
#if defined(HAVE_OMP_3_1)

#pragma omp atomic capture
   {
      old = value;
      value += increment;
   }

#elif defined(__GNUC__) && defined(_OPENMP)

   return __sync_fetch_and_add(&value, increment);
#else

#ifdef _OPENMP
#pragma omp critical
#endif
   {
      old = value;
      value += increment;
   }
#endif

   return old;
}

#endif
