// Core/Debug.h

#ifndef _CORE_DEBUG_H_INCLUDED_
#define _CORE_DEBUG_H_INCLUDED_

#include <Core/Windows.h>

#include <string>

#ifdef _MSC_VER

namespace Core
{
	inline void WriteToOutput(const char* str)
	{
		OutputDebugStringA(str);
	}

	inline void WriteToOutput(const std::string& str)
	{
		OutputDebugStringA(str.c_str());
	}
}

#endif

#endif