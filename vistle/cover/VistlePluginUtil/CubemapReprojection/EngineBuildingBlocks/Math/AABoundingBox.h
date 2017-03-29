// EngineBuildingBlocks/Math/AABoundingBox.h

#ifndef _ENGINEBUILDINGBLOCKS_AABOUNDINGBOX_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_AABOUNDINGBOX_H_INCLUDED_

#include <EngineBuildingBlocks/Math/GLM.h>

#include <limits>

namespace EngineBuildingBlocks
{
	namespace Math
	{
		enum class BoxPlane : unsigned char
		{
			Left = 0, Right, Bottom, Top, Back, Front, COUNT
		};

		enum class BoxCorner : unsigned char
		{
			BackBottomLeft = 0, BackBottomRight, BackTopLeft, BackTopRight,
			FrontBottomLeft, FrontBottomRight, FrontTopLeft, FrontTopRight, COUNT
		};

		const unsigned c_CountBoxPlanes = 6;
		const unsigned c_CountBoxCorners = 8;

		struct BoxCorners
		{
			glm::vec3 Corners[c_CountBoxCorners];
		};

		struct AABoundingBox
		{
			glm::vec3 Minimum;
			glm::vec3 Maximum;

			bool IsValid() const;
			bool IsNonZero() const;

			glm::vec3 GetCenter() const;
			glm::vec3 GetSize() const;
			float GetLongestEdge() const;
			float GetSmallestEdge() const;

			float GetSurfaceSize() const;
			glm::vec3 GetSurfaceSizes() const;
			float GetVolumeSize() const;

			void Scale(const glm::vec3& scaler);
			void Scale(float scaler);

			// This function augments the size (NOT the half size!) of the bounding box
			// with given addition vector.
			void Augment(const glm::vec3& addition);

			// This function augments the size (NOT the half size!) of the bounding box
			// with given addition value in each dimensions.
			void Augment(float addition);

			glm::vec3 GetClosestPoint(const glm::vec3& referencePoint) const;
			float GetDistance(const glm::vec3& referencePoint) const;
			float GetBoundaryDistance(const glm::vec3& referencePoint) const;

			AABoundingBox Transform(const glm::mat4x3& m) const;

			bool IsContaining(const glm::vec3& point) const;

			BoxCorners GetBoxCorners() const;
			void GetBoxCorners(BoxCorners& boxCorners) const;

			static AABoundingBox GetBoundingBox(const glm::vec3* positions, unsigned countPositions);

			static const unsigned c_CountVertices = (unsigned)BoxCorner::COUNT;
			static const unsigned c_CountTriangles = 2 * (unsigned)BoxPlane::COUNT;
			static const unsigned c_CountIndices = c_CountTriangles * 3;

			static const glm::vec3* GetGeneralVertices();
			static const unsigned* Get8VertexIndices();

			static AABoundingBox Union(const AABoundingBox& box0, const AABoundingBox& box1);
		};

		const AABoundingBox c_UnitAABB { glm::vec3(-0.5f), glm::vec3(0.5f) };

		const AABoundingBox c_InvalidAABB
		{
			glm::vec3(std::numeric_limits<float>::max()),
			glm::vec3(-std::numeric_limits<float>::max())
		};
	}
}

#endif