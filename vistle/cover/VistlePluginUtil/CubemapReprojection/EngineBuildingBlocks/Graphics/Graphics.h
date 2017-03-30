// EngineBuildingBlocks/Graphics/Graphics.h

#ifndef _ENGINEBUILDINGBLOCKS_GRAPHICS_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_GRAPHICS_H_INCLUDED_

#include <cassert>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		inline unsigned GetThreadGroupCount(unsigned globalSize, unsigned localSize)
		{
			assert(globalSize > 0 && localSize > 0);
			return (globalSize - 1) / localSize + 1;
		}
	}
}

#endif