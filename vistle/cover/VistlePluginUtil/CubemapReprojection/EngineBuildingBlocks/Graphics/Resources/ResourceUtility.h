// EngineBuildingBlocks/Graphics/Resources/ResourceUtility.h

#ifndef _ENGINEBUILDINGBLOCKS_RESOURCEUTILITY_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_RESOURCEUTILITY_H_INCLUDED_

#include <algorithm>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		inline unsigned GetMipmapLevelCount(unsigned width, unsigned height)
		{
			unsigned count = 1;
			for (unsigned size = std::max(width, height); size > 1; size >>= 1, ++count);
			return count;
		}
	}
}

#endif