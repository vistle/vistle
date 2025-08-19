#pragma once

// Replacement for shared library/DLL header file to support static library creation and use.
// See http://gcc.gnu.org/wiki/Visibility for more information on GCC symbol visibility.

#if defined BUILD_SHARED_LIBRARIES

  #if defined _WIN32
      #define tecio_HELPER_DLL_IMPORT __declspec(dllimport)
      #define tecio_HELPER_DLL_EXPORT __declspec(dllexport)
      #define tecio_HELPER_DLL_LOCAL
  #else
      #if __GNUC__ >= 4
          #define tecio_HELPER_DLL_IMPORT __attribute__ ((visibility("default")))
          #define tecio_HELPER_DLL_EXPORT __attribute__ ((visibility("default")))
          #define tecio_HELPER_DLL_LOCAL  __attribute__ ((visibility("hidden")))
      #else
          #define tecio_HELPER_DLL_IMPORT
          #define tecio_HELPER_DLL_EXPORT
          #define tecio_HELPER_DLL_LOCAL
      #endif
  #endif

  #ifdef tecio_EXPORTS // defined if we are building the tecio code (instead of using it)
    #define tecio_API tecio_HELPER_DLL_EXPORT
  #else
    #define tecio_API tecio_HELPER_DLL_IMPORT
  #endif // tecio_EXPORTS

#else // BUILD_SHARED_LIBRARIES

  #define tecio_API

#endif // BUILD_SHARED_LIBRARIES

