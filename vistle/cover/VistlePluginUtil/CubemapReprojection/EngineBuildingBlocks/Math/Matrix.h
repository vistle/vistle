// EngineBuildingBlocks/Math/Matrix.h

#ifndef _ENGINEBUILDINGBLOCKS_MATRIX_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_MATRIX_H_INCLUDED_

#include <EngineBuildingBlocks/Math/GLM.h>

#include <algorithm>
#include <math.h>

namespace EngineBuildingBlocks
{
	inline bool IsEqual(const float* a1, const float* a2, float epsilon, unsigned size)
	{
		float maxDiff = 0.0f;
		auto end = a1 + size;
		for (; a1 != end; ++a1, ++a2)
		{
			maxDiff = std::max(maxDiff, fabsf(*a1 - *a2));
		}
		return (maxDiff <= epsilon);
	}

	inline bool IsEqual(const glm::mat4& m1, const glm::mat4& m2, float epsilon)
	{
		return IsEqual(glm::value_ptr(m1), glm::value_ptr(m2), epsilon, 16U);
	}
}

#endif