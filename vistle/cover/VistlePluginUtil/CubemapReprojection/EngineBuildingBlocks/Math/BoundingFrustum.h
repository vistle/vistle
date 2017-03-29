// EngineBuildingBlocks/Math/BoundingFrustum.h

#ifndef _ENGINEBUILDINGBLOCKS_BOUNDINGFRUSTUM_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_BOUNDINGFRUSTUM_H_INCLUDED_

#include <EngineBuildingBlocks/Math/Plane.h>
#include <EngineBuildingBlocks/Graphics/Camera/CameraProjection.h>

namespace EngineBuildingBlocks
{
	namespace Math
	{
		enum class FrustumPlane : unsigned char
		{
			Left = 0, Right, Bottom, Top, Near, Far
		};

		enum class FrustumCorner : unsigned char
		{
			NearBottomLeft = 0, NearBottomRight, NearTopLeft, NearTopRight,
			FarBottomLeft, FarBottomRight, FarTopLeft, FarTopRight
		};

		const unsigned c_CountFrustumPlanes = 6U;
		const unsigned c_CountFrustumCorners = 8U;

		struct FrustumPlanes
		{
			Plane Planes[c_CountFrustumPlanes];
		};

		struct FrustumCorners
		{
			glm::vec3 Corners[c_CountFrustumCorners];
		};


		class BoundingFrustum
		{
			glm::mat4 m_Matrix;
			FrustumPlanes m_Planes;
			FrustumCorners m_Corners;
			Graphics::CameraProjection m_Projection;
			Graphics::ProjectionType m_ProjectionType;

			void SetPlanesAndCorners();

			bool IsProjectingTo_0_1_Interval() const;

		public:

			BoundingFrustum();
			BoundingFrustum(const glm::mat4& matrix,
				Graphics::CameraProjection projection);

			void SetData(const glm::mat4& matrix,
				Graphics::CameraProjection projection);

			const FrustumPlanes& GetPlanes() const;
			const FrustumCorners& GetCorners() const;

			const Plane& GetPlane(FrustumPlane plane) const;
			const glm::vec3& GetCorner(FrustumCorner corner) const;

			glm::vec3 GetPlaneMiddle(FrustumPlane planeId) const;
			glm::vec3 GetPlaneBottom(FrustumPlane planeId) const;

			// Returns direction and up according to the following convention:
			//
			//         up ^
			//            |
			//  \---------+---------/
			//   \        |        /
			//    \       |/      /
			//  ---\------+------/---> direction
			//      \    /|     /
			//       \  / |    /
			//        \/--|---/
			//        /   |
			//       /
			//      L right = normal
			//
			void GetPlaneDirectionUp(FrustumPlane planeId, glm::vec3& direction, glm::vec3& up) const;

			float GetPlaneUpExtent(FrustumPlane planeId) const;

			glm::vec2 GetNearPlaneSize() const;
			void GetNearPlaneOrientation(glm::vec3& direction, glm::vec3& up, glm::vec3& right) const;

			void GetSidePointsAlongDirection(const glm::vec3& direction, glm::vec3& min,
				glm::vec3& max) const;
		};
	}
}

#endif