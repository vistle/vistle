// EngineBuildingBlocks/Math/Plane.h

#ifndef _ENGINEBUILDINGBLOCKS_PLANE_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_PLANE_H_INCLUDED_

#include <EngineBuildingBlocks/Math/GLM.h>

namespace EngineBuildingBlocks
{
	namespace Math
	{
		struct Plane
		{
			glm::vec3 Normal;
			float D;

		public:

			inline void SetFromNormalAndPoint(const glm::vec3& normal, const glm::vec3& p)
			{
				Normal = glm::normalize(normal);
				D = -glm::dot(Normal, p);
			}

			inline void SetFromPoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
			{
				SetFromNormalAndPoint(glm::cross(p2 - p1, p3 - p1), p1);
			}

			inline float Evaluate(const glm::vec3& p)
			{
				return glm::dot(Normal, p) + D;
			}
		};
	}
}

#endif