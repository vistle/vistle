// EngineBuildingBlocks/Math/AABoundingBox.cpp

#include <EngineBuildingBlocks/Math/AABoundingBox.h>

#include <algorithm>

using namespace EngineBuildingBlocks::Math;

unsigned c_8VertexIndices[] =
{
	2, 0, 6, // Left
	6, 0, 4,

	7, 1, 3, // Right
	5, 1, 7,

	0, 5, 4, // Bottom
	1, 5, 0,

	6, 7, 2, // Top
	2, 7, 3,

	2, 1, 0, // Back
	3, 1, 2,

	4, 5, 6, // Front
	6, 5, 7
};

glm::vec3 c_GeneralVertices[] =
{
	glm::vec3(-0.5f, -0.5f, -0.5f),
	glm::vec3(0.5f, -0.5f, -0.5f),
	glm::vec3(-0.5f, 0.5f, -0.5f),
	glm::vec3(0.5f, 0.5f, -0.5f),
	glm::vec3(-0.5f, -0.5f, 0.5f),
	glm::vec3(0.5f, -0.5f, 0.5f),
	glm::vec3(-0.5f, 0.5f, 0.5f),
	glm::vec3(0.5f, 0.5f, 0.5f)
};

const glm::vec3* AABoundingBox::GetGeneralVertices()
{
	return c_GeneralVertices;
}

const unsigned* AABoundingBox::Get8VertexIndices()
{
	return c_8VertexIndices;
}

bool AABoundingBox::IsValid() const
{
	return (Minimum.x <= Maximum.x && Minimum.y <= Maximum.y && Minimum.z <= Maximum.z);
}

bool AABoundingBox::IsNonZero() const
{
	return (Minimum.x < Maximum.x && Minimum.y < Maximum.y && Minimum.z < Maximum.z);
}

glm::vec3 AABoundingBox::GetCenter() const
{
	return (Minimum + Maximum) / 2.0f;
}

glm::vec3 AABoundingBox::GetSize() const
{
	return (Maximum - Minimum);
}

float AABoundingBox::GetLongestEdge() const
{
	auto size = GetSize();
	return std::max(std::max(size.x, size.y), size.z);
}

float AABoundingBox::GetSmallestEdge() const
{
	auto size = GetSize();
	return std::min(std::min(size.x, size.y), size.z);
}

float AABoundingBox::GetSurfaceSize() const
{
	float x = Maximum.x - Minimum.x;
	float y = Maximum.y - Minimum.y;
	float z = Maximum.z - Minimum.z;
	return 2.0f * (x * (y + z) + y * z);
}

glm::vec3 AABoundingBox::GetSurfaceSizes() const
{
	float x = Maximum.x - Minimum.x;
	float y = Maximum.y - Minimum.y;
	float z = Maximum.z - Minimum.z;
	return glm::vec3(x, y, z);
}

float AABoundingBox::GetVolumeSize() const
{
	glm::vec3 diff = Maximum - Minimum;
	return diff.x * diff.y * diff.z;
}

void AABoundingBox::Scale(const glm::vec3& scaler)
{
	glm::vec3 center = (Minimum + Maximum) * 0.5f;
	glm::vec3 halfSize = (Maximum - Minimum) * scaler * 0.5f;
	Minimum = center - halfSize;
	Maximum = center + halfSize;
}

void AABoundingBox::Scale(float scaler)
{
	Scale(glm::vec3(scaler));
}

void AABoundingBox::Augment(const glm::vec3& addition)
{
	glm::vec3 center = (Minimum + Maximum) * 0.5f;
	glm::vec3 halfSize = (Maximum - Minimum + addition) * 0.5f;
	Minimum = center - halfSize;
	Maximum = center + halfSize;
}

void AABoundingBox::Augment(float addition)
{
	Augment(glm::vec3(addition));
}

glm::vec3 AABoundingBox::GetClosestPoint(const glm::vec3& referencePoint) const
{
	return glm::clamp(referencePoint, Minimum, Maximum);
}

float AABoundingBox::GetDistance(const glm::vec3& referencePoint) const
{
	glm::vec3 closestBoxPoint = glm::clamp(referencePoint, Minimum, Maximum);
	return glm::length(closestBoxPoint - referencePoint);
}

float AABoundingBox::GetBoundaryDistance(const glm::vec3& referencePoint) const
{
	glm::vec3 closestBoxPoint = glm::clamp(referencePoint, Minimum, Maximum);
	if (closestBoxPoint == referencePoint)
	{
		auto minDistance = glm::min(referencePoint - Minimum, Maximum - referencePoint);
		return std::min(minDistance.x, std::min(minDistance.y, minDistance.z));
	}
	return glm::length(closestBoxPoint - referencePoint);
}



AABoundingBox AABoundingBox::Transform(const glm::mat4x3& m) const
{
	auto corners = GetBoxCorners().Corners;
	glm::vec3 x[c_CountBoxCorners];
	auto& a = reinterpret_cast<const glm::mat3&>(m);
	auto& p = m[3];
	for (unsigned i = 0; i < c_CountBoxCorners; i++)
	{
		x[i] = a * corners[i] + p;
	}
	return GetBoundingBox(x, c_CountBoxCorners);
}

bool AABoundingBox::IsContaining(const glm::vec3& point) const
{
	return (point.x >= Minimum.x && point.x <= Maximum.x
		&& point.y >= Minimum.y && point.y <= Maximum.y
		&& point.z >= Minimum.z && point.z <= Maximum.z);
}

BoxCorners AABoundingBox::GetBoxCorners() const
{
	BoxCorners boxCorners;
	GetBoxCorners(boxCorners);
	return boxCorners;
}

void AABoundingBox::GetBoxCorners(BoxCorners& boxCorners) const
{
	auto& corners = boxCorners.Corners;
	corners[(unsigned)BoxCorner::BackBottomLeft] = Minimum;
	corners[(unsigned)BoxCorner::BackBottomRight] = glm::vec3(Maximum.x, Minimum.y, Minimum.z);
	corners[(unsigned)BoxCorner::BackTopLeft] = glm::vec3(Minimum.x, Maximum.y, Minimum.z);
	corners[(unsigned)BoxCorner::BackTopRight] = glm::vec3(Maximum.x, Maximum.y, Minimum.z);
	corners[(unsigned)BoxCorner::FrontBottomLeft] = glm::vec3(Minimum.x, Minimum.y, Maximum.z);
	corners[(unsigned)BoxCorner::FrontBottomRight] = glm::vec3(Maximum.x, Minimum.y, Maximum.z);
	corners[(unsigned)BoxCorner::FrontTopLeft] = glm::vec3(Minimum.x, Maximum.y, Maximum.z);
	corners[(unsigned)BoxCorner::FrontTopRight] = Maximum;
}

AABoundingBox AABoundingBox::GetBoundingBox(const glm::vec3* positions, unsigned countPositions)
{
	glm::vec3 min = c_InvalidAABB.Minimum;
	glm::vec3 max = c_InvalidAABB.Maximum;
	for (unsigned i = 0; i < countPositions; ++i)
	{
		auto& position = positions[i];
		min = glm::min(min, position);
		max = glm::max(max, position);
	}
	return{ min, max };
}

AABoundingBox AABoundingBox::Union(const AABoundingBox& box0, const AABoundingBox& box1)
{
	return{ glm::min(box0.Minimum, box1.Minimum), glm::max(box0.Maximum, box1.Maximum) };
}