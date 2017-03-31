// EngineBuildingBlocks/Math/Matrix.h

#ifndef _ENGINEBUILDINGBLOCKS_MATRIX_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_MATRIX_H_INCLUDED_

#include <EngineBuildingBlocks/Math/GLM.h>

#include <algorithm>
#include <math.h>
#include <stdio.h>

namespace EngineBuildingBlocks
{
	inline void PrintMatrix(const glm::mat4& m)
	{
		printf("%6g %6g %6g %6g\n%6g %6g %6g %6g\n%6g %6g %6g %6g\n%6g %6g %6g %6g\n",
			m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1], m[0][2], m[1][2], m[2][2], m[3][2], m[0][3], m[1][3], m[2][3], m[3][3]);
	}

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