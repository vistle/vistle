// CubemapStreaming_Client/Message.cpp

#include <Message.h>
#include <EngineBuildingBlocks/Graphics/Camera/CubemapHelper.hpp>

#include <cmath>

using namespace EngineBuildingBlocks::Graphics;

namespace CubemapStreaming
{
	unsigned Server_CubemapUpdateData::GetImageDataSize() const
	{
		return CubemapStreaming::GetImageDataSize(TextureWidth, TextureHeight, IsContainingAlpha,
			SideVisiblePortion, PosZVisiblePortion);
	}

	void ValidateDimensions(unsigned width, unsigned height,
		float sideVisiblePortion, float posZVisiblePortion)
	{
		assert(sideVisiblePortion >= 0.0f && sideVisiblePortion <= 1.0f);
		assert(posZVisiblePortion >= 0.0f && posZVisiblePortion <= 1.0f);
		assert(sideVisiblePortion == 1.0f || posZVisiblePortion == 0.0f);
		assert(width > 0 && height > 0);
		assert(width % 8 == 0 && height % 2 == 0);
		assert(posZVisiblePortion == 0.0f || posZVisiblePortion == 1.0f);
	}

	void GetImageDataSideBounds(unsigned width, unsigned height, float sideVisiblePortion, float posZVisiblePortion,
		glm::uvec2* start, glm::uvec2* end)
	{
		unsigned w = width;
		unsigned h = height;
		float p0 = sideVisiblePortion;
		//float p1 = posZVisiblePortion;
		unsigned ws0 = unsigned(roundf((float)w * (1.0f - p0)) / 4.0f) * 4;
		unsigned hs0 = unsigned(roundf((float)h * (1.0f - p0)));
		unsigned we0 = unsigned(roundf((float)w * p0) / 4.0f) * 4;
		unsigned he0 = unsigned(roundf((float)h * p0));
		start[0] = glm::uvec2(0, 0); end[0] = glm::uvec2(we0, h);
		start[1] = glm::uvec2(ws0, 0); end[1] = glm::uvec2(w, h);
		start[2] = glm::uvec2(0, hs0); end[2] = glm::uvec2(w, h);
		start[3] = glm::uvec2(0, 0); end[3] = glm::uvec2(w, he0);
		start[4] = glm::uvec2(0, 0);
		start[5] = glm::uvec2(0, 0); end[5] = glm::uvec2(w, h);

		// Currently only supporting empty or complete PosZ data.
		if (posZVisiblePortion == 0.0f)
		{
			end[4] = glm::uvec2(0, 0);
		}
		else
		{
			end[4] = glm::uvec2(w, h);
		}
	}

	void GetImageDataSideSize(unsigned width, unsigned height, float sideVisiblePortion, float posZVisiblePortion,
		glm::uvec2* sizes)
	{
		glm::uvec2 sideStarts[c_CountCubemapSides], sideEnds[c_CountCubemapSides];
		GetImageDataSideBounds(width, height, sideVisiblePortion, posZVisiblePortion,
			sideStarts, sideEnds);
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
			sizes[i] = (glm::uvec2)glm::abs((glm::ivec2)sideEnds[i] - (glm::ivec2)sideStarts[i]);
	}

	inline unsigned GetPixelCount(glm::uvec2* start, glm::uvec2* end, unsigned side)
	{
		auto diff = end[side] - start[side];
		return diff.x * diff.y;
	}

	unsigned GetImageDataSize(unsigned width, unsigned height, bool isContainingAlpha,
		float sideVisiblePortion, float posZVisiblePortion)
	{
		glm::uvec2 start[c_CountCubemapSides], end[c_CountCubemapSides];
		GetImageDataSideBounds(width, height, sideVisiblePortion, posZVisiblePortion, start, end);
		unsigned countPixels = 0;
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			countPixels += GetPixelCount(start, end, i);
		}

		return countPixels * ((isContainingAlpha ? 4 : 3) * (unsigned)sizeof(unsigned char) + (unsigned)sizeof(float));
	}

	void GetImageDataOffsets(unsigned width, unsigned height, bool isContainingAlpha,
		float sideVisiblePortion, float posZVisiblePortion, unsigned* colorOffsets, unsigned* depthOffsets)
	{
		glm::uvec2 start[c_CountCubemapSides], end[c_CountCubemapSides];
		GetImageDataSideBounds(width, height, sideVisiblePortion, posZVisiblePortion, start, end);
		unsigned colorElementSize = (isContainingAlpha ? 4 : 3) * (unsigned)sizeof(unsigned char);
		unsigned depthElementSize = (unsigned)sizeof(float);
		unsigned offset = 0;
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			colorOffsets[i] = offset;
			offset += GetPixelCount(start, end, i) * colorElementSize;
		}
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			depthOffsets[i] = offset;
			offset += GetPixelCount(start, end, i) * depthElementSize;
		}
	}
}