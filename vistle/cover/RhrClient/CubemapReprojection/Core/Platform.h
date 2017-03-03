// Platform.h

#ifndef _CORE_PLATFORM_H_INCLUDED_
#define _CORE_PLATFORM_H_INCLUDED_

/////////////////////////////////////////////// OPERATING SYSTEM ///////////////////////////////////////////////

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define IS_WINDOWS
#include <SDKDDKVer.h>
#endif

/////////////////////////////////////////////// COMPILER ///////////////////////////////////////////////

#if defined(_MSC_VER) // MSVC.

#define CORE_FORCEINLINE __forceinline

#if (_MSC_VER >= 1900)

#define CORE_NOEXCEPT noexcept
#define CORE_NOEXCEPT_OP(x)	noexcept(x)
#define CORE_CONSTEXPR constexpr

#define HAS_DELETE_DEFAULT_FUNCTION_DECLARATIONS

#else // Older MSVC.

#define CORE_NOEXCEPT	throw ()
#define CORE_NOEXCEPT_OP(x)
#define CORE_CONSTEXPR

#endif // if (_MSC_VER >= 1900)

#elif defined(__GNUC__) // GCC.

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#define CORE_NOEXCEPT noexcept
#define CORE_NOEXCEPT_OP(x) noexcept(x)
#define CORE_CONSTEXPR constexpr
#define CORE_FORCEINLINE

#define HAS_DELETE_DEFAULT_FUNCTION_DECLARATIONS

#else // Other compiler.

#define CORE_NOEXCEPT noexcept
#define CORE_NOEXCEPT_OP(x) noexcept(x)
#define CORE_CONSTEXPR constexpr
#define CORE_FORCEINLINE

#define HAS_DELETE_DEFAULT_FUNCTION_DECLARATIONS

#endif // Compiler branch.

/////////////////////////////////////////////// TARGET PLATFORM ///////////////////////////////////////////////

#if defined(_M_IX86)
#define TARGET_PLATFORM_X86
#endif

#if defined(_M_AMD64)
#define TARGET_PLATFORM_X64
#endif

#if defined(_M_ARM)
#define TARGET_PLATFORM_ARM
#endif

#endif // ifndef _CORE_PLATFORM_H_