// Core/Windows.h

#ifndef _CORE_WINDOWS_H_INCLUDED_
#define _CORE_WINDOWS_H_INCLUDED_

#include <Core/Platform.h>

#if defined(IS_WINDOWS) && defined(_MSC_VER)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#endif // if defined(IS_WINDOWS) && defined(_MSC_VER)

#endif // ifndef _CORE_WINDOWS_H_INCLUDED_