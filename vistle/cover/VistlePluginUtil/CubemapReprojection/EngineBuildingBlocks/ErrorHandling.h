// EngineBuildingBlocks/ErrorHandling.h

#ifndef _ENGINEBUILDINGBLOCKS_ERRORHANDLING_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_ERRORHANDLING_H_INCLUDED_

#include <stdexcept>
#include <sstream>
#include <iostream>

#include <Core/Debug.h>

namespace EngineBuildingBlocks
{
	inline void RaiseException(const char* str)
	{
		throw std::runtime_error(str);
	}

	inline void RaiseException(const std::string& str)
	{
		RaiseException(str.c_str());
	}

	inline void RaiseException(const std::stringstream& ss)
	{
		RaiseException(ss.str().c_str());
	}

	inline void RaiseWarning(const char* str)
	{
		printf("[WARNING] :: %s\n", str);
#ifdef _MSC_VER
		Core::WriteToOutput(str);
#endif
	}

	inline void RaiseWarning(const std::string& str)
	{
		RaiseWarning(str.c_str());
	}

	inline void RaiseWarning(const std::stringstream& ss)
	{
		RaiseWarning(ss.str().c_str());
	}
}

#endif