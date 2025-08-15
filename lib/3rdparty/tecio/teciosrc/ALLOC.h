 #ifndef ALLOC_H
 #define ALLOC_H
 #if defined __cplusplus
#include <new>
 #if defined TRACK_MEMORY_USAGE
#include <malloc.h>
 #endif
 #endif
#include "CodeContract.h"
 #if !defined __cplusplus
 #define ___23(N,___4234,str) (___4234 *)malloc((N)*sizeof(___4234))
 #define ALLOC_ITEM(___4234,str)    (___4234 *)malloc(sizeof(___4234))
 #ifdef _DEBUG
 #define ___1528(X,str)  do { free((void *)(X)); *((void **)&(X)) = (void *)0xFFFF; } while (0)
 #define ___1529(X,str)   do { free((void *)(X)); *((void **)&(X)) = (void *)0xFFFF; } while (0)
 #else
 #define ___1528(X,str)  free((void *)(X))
 #define ___1529(X,str)   free((void *)(X))
 #endif
 #else
 #ifdef TRACK_MEMORY_USAGE
void printMemoryUsage(char const* context, size_t      sizeChange); void ___1934(); void ___489(); void ___4205(size_t size); void ___4207(size_t size); void ___4206(); void ___4208(); void ___1756(size_t* ___2405, size_t* ___2404, size_t* ___2406, size_t* ___2407);
 #endif
 #if defined MSWIN && defined _DEBUG && defined TRACK_MEMORY_USAGE
template <typename T> inline T *nonExceptionNew(size_t      ___2810, const char* ___1392, int         lineNumber) { REQUIRE(___2810 > 0); REQUIRE(VALID_REF(___1392)); REQUIRE(lineNumber > 0); T* ___3356 = NULL; try {
 #ifdef DEBUG_NEW
 #ifdef new
 #undef new
 #define USING_DEBUG_NEW
 #endif
___3356 = new(___1392, lineNumber) T[___2810];
 #ifdef USING_DEBUG_NEW
 #undef USING_DEBUG_NEW
 #endif
 #else
___3356 = new T[___2810];
 #endif
} catch (std::bad_alloc&) { ___3356 = NULL; }
 #ifdef TRACK_MEMORY_USAGE
if (___3356 != NULL) {
 #ifdef MSWIN
___4205(_msize(___3356));
 #else
___4205(malloc_usable_size(___3356));
 #endif
}
 #endif
ENSURE(VALID_REF_OR_NULL(___3356)); return ___3356; }
 #define ___23(N,___4234,str) nonExceptionNew<___4234>((N),__FILE__,__LINE__)
 #else
template <typename T> inline T *nonExceptionNew(size_t ___2810) { REQUIRE(___2810 > 0); T *___3356 = NULL; try { ___3356 = new T[___2810]; } catch (std::bad_alloc&) { ___3356 = NULL; }
 #ifdef TRACK_MEMORY_USAGE
if (___3356 != NULL) {
 #ifdef MSWIN
___4205(_msize(___3356));
 #else
___4205(malloc_usable_size((void*)___3356));
 #endif
}
 #endif
return ___3356; }
 #define ___23(N,___4234,str) nonExceptionNew<___4234>((N))
 #endif
 #define ALLOC_ITEM(___4234,str)    ___23(1,___4234,str)
template <typename T> inline void nonExceptionDelete(T* &___3249) { REQUIRE(VALID_REF(___3249));
 #if defined TRACK_MEMORY_USAGE
{ if (___3249 != NULL) {
 #ifdef MSWIN
___4207(_msize(___3249));
 #else
___4207(malloc_usable_size((void *)___3249));
 #endif
} }
 #endif
delete [] ___3249;
 #if !defined NO_ASSERTS
___3249 = (T*)(void*)0xFFFF;
 #endif
} template <typename T> inline void nonExceptionDelete(T* const& ___3249) { nonExceptionDelete(const_cast<T*&>(___3249)); }
 #define ___1528(___3249,str)  nonExceptionDelete((___3249))
 #define ___1529(___3249,str)   ___1528(___3249,str)
 #endif
struct ___954 { template<typename T> void operator()(T*& object) { delete object; object = NULL; } };
 #endif 
