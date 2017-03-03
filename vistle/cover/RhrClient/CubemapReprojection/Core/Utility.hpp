// Core/Utility.hpp

#ifndef _CORE_UTILITY_HPP_INCLUDED_
#define _CORE_UTILITY_HPP_INCLUDED_

#include <cassert>

namespace Core
{
	template <typename T>
	inline bool IsPowerOfTwo(T x)
	{
		return ((x > 0) && (x & (x - 1)) == 0);
	}

	template <typename T>
	inline T AlignSize(T size, T alignment)
	{
		assert(IsPowerOfTwo(alignment));
		T alignMask = alignment - 1;
		return ((size + alignMask) & ~alignMask);
	}
}

#endif