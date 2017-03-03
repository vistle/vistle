// EngineBuildingBlocks/Math/BoundingFrustum.cpp

#include <EngineBuildingBlocks/Math/BoundingFrustum.h>
#include <EngineBuildingBlocks/ErrorHandling.h>

using namespace EngineBuildingBlocks::Math;

BoundingFrustum::BoundingFrustum()
{
}

BoundingFrustum::BoundingFrustum(const glm::mat4& matrix,
	EngineBuildingBlocks::Graphics::CameraProjection projection)
{
	SetData(matrix, projection);
}

void BoundingFrustum::SetPlanesAndCorners()
{
	auto& planes = m_Planes.Planes;
	auto& corners = m_Corners.Corners;

	float projectedNear = (IsProjectingTo_0_1_Interval() ? 0.0f : -1.0f);

	glm::mat4 mInv = glm::inverse(m_Matrix);
	glm::vec3 ssPoints[] =
	{
		glm::vec3(-1.0f, -1.0f, projectedNear),	// Near bottom left.
		glm::vec3(1.0f, -1.0f, projectedNear),	// Near bottom right.
		glm::vec3(-1.0f, 1.0f, projectedNear),	// Near top left.
		glm::vec3(1.0f, 1.0f, projectedNear),	// Near top right.
		glm::vec3(-1.0f, -1.0f, 1.0f),			// Far bottom left.
		glm::vec3(1.0f, -1.0f, 1.0f),			// Far bottom right.
		glm::vec3(-1.0f, 1.0f, 1.0f),			// Far top left.
		glm::vec3(1.0f, 1.0f, 1.0f)				// Far top right.
	};
	for (unsigned i = 0; i < c_CountFrustumCorners; i++)
	{
		glm::vec4 worldPoint = mInv * glm::vec4(ssPoints[i], 1.0f);
		corners[i] = glm::vec3(worldPoint.x / worldPoint.w, worldPoint.y / worldPoint.w,
			worldPoint.z / worldPoint.w);
	}

	planes[(int)FrustumPlane::Left].SetFromPoints(corners[(int)FrustumCorner::NearBottomLeft],
		corners[(int)FrustumCorner::NearTopLeft], corners[(int)FrustumCorner::FarBottomLeft]);
	planes[(int)FrustumPlane::Right].SetFromPoints(corners[(int)FrustumCorner::NearTopRight],
		corners[(int)FrustumCorner::NearBottomRight], corners[(int)FrustumCorner::FarBottomRight]);
	planes[(int)FrustumPlane::Bottom].SetFromPoints(corners[(int)FrustumCorner::NearBottomLeft],
		corners[(int)FrustumCorner::FarBottomRight], corners[(int)FrustumCorner::NearBottomRight]);
	planes[(int)FrustumPlane::Top].SetFromPoints(corners[(int)FrustumCorner::NearTopLeft],
		corners[(int)FrustumCorner::NearTopRight], corners[(int)FrustumCorner::FarTopRight]);
	planes[(int)FrustumPlane::Near].SetFromPoints(corners[(int)FrustumCorner::NearBottomLeft],
		corners[(int)FrustumCorner::NearBottomRight], corners[(int)FrustumCorner::NearTopLeft]);
	planes[(int)FrustumPlane::Far].SetFromPoints(corners[(int)FrustumCorner::FarBottomLeft],
		corners[(int)FrustumCorner::FarTopLeft], corners[(int)FrustumCorner::FarBottomRight]);
}

bool BoundingFrustum::IsProjectingTo_0_1_Interval() const
{
	if (m_ProjectionType == Graphics::ProjectionType::Perspective)
	{
		return m_Projection.Projection.Perspective.IsProjectingTo_0_1_Interval;
	}
	else if (m_ProjectionType == Graphics::ProjectionType::Orthographic)
	{
		return m_Projection.Projection.Orthographic.IsProjectingTo_0_1_Interval;
	}
	return false;
}

void BoundingFrustum::SetData(const glm::mat4& matrix,
	EngineBuildingBlocks::Graphics::CameraProjection projection)
{
	auto projectionType = projection.Type;

	this->m_Matrix = matrix;
	this->m_Projection = projection;
	this->m_ProjectionType = projectionType;

	if(projectionType != Graphics::ProjectionType::Perspective
		&& projectionType != Graphics::ProjectionType::Orthographic)
	{
		RaiseException("Unknown projection type.");
	}

	SetPlanesAndCorners();
}

const FrustumPlanes& BoundingFrustum::GetPlanes() const
{
	return m_Planes;
}

const FrustumCorners& BoundingFrustum::GetCorners() const
{
	return m_Corners;
}

const Plane& BoundingFrustum::GetPlane(FrustumPlane plane) const
{
	return m_Planes.Planes[(unsigned)plane];
}

const glm::vec3& BoundingFrustum::GetCorner(FrustumCorner corner) const
{
	return m_Corners.Corners[(unsigned)corner];
}

glm::vec3 BoundingFrustum::GetPlaneMiddle(FrustumPlane planeId) const
{
	auto& corners = m_Corners.Corners;
	switch (planeId)
	{
	case FrustumPlane::Left:
		return (corners[(int)FrustumCorner::NearBottomLeft] + corners[(int)FrustumCorner::NearTopLeft]
			+ corners[(int)FrustumCorner::FarBottomLeft] + corners[(int)FrustumCorner::FarTopLeft]) / 4.0f;
	case FrustumPlane::Right:
		return (corners[(int)FrustumCorner::NearBottomRight] + corners[(int)FrustumCorner::NearTopRight]
			+ corners[(int)FrustumCorner::FarBottomRight] + corners[(int)FrustumCorner::FarTopRight]) / 4.0f;
	case FrustumPlane::Bottom:
		return (corners[(int)FrustumCorner::NearBottomLeft] + corners[(int)FrustumCorner::NearBottomRight]
			+ corners[(int)FrustumCorner::FarBottomLeft] + corners[(int)FrustumCorner::FarBottomRight]) / 4.0f;
	case FrustumPlane::Top:
		return (corners[(int)FrustumCorner::NearTopLeft] + corners[(int)FrustumCorner::NearTopRight]
			+ corners[(int)FrustumCorner::FarTopLeft] + corners[(int)FrustumCorner::FarTopRight]) / 4.0f;
	case FrustumPlane::Near:
		return (corners[(int)FrustumCorner::NearBottomLeft] + corners[(int)FrustumCorner::NearTopRight]) / 2.0f;
	case FrustumPlane::Far:
		return (corners[(int)FrustumCorner::FarBottomLeft] + corners[(int)FrustumCorner::FarTopRight]) / 2.0f;
	}
	return glm::vec3();
}

glm::vec3 BoundingFrustum::GetPlaneBottom(FrustumPlane planeId) const
{
	auto& corners = m_Corners.Corners;
	switch (planeId)
	{
	case FrustumPlane::Left:
		return (corners[(int)FrustumCorner::NearBottomLeft]
			+ corners[(int)FrustumCorner::NearTopLeft]) / 2.0f;
	case FrustumPlane::Right:
		return (corners[(int)FrustumCorner::NearBottomRight]
			+ corners[(int)FrustumCorner::NearTopRight]) / 2.0f;
	case FrustumPlane::Bottom:
		return (corners[(int)FrustumCorner::NearBottomLeft]
			+ corners[(int)FrustumCorner::NearBottomRight]) / 2.0f;
	case FrustumPlane::Top:
		return (corners[(int)FrustumCorner::NearTopLeft]
			+ corners[(int)FrustumCorner::NearTopRight]) / 2.0f;
	case FrustumPlane::Near:
		return (corners[(int)FrustumCorner::NearBottomLeft] + corners[(int)FrustumCorner::NearBottomRight]) / 2.0f;
	case FrustumPlane::Far:
		return (corners[(int)FrustumCorner::FarBottomLeft] + corners[(int)FrustumCorner::FarBottomRight]) / 2.0f;
	}
	return glm::vec3();
}

void BoundingFrustum::GetPlaneDirectionUp(FrustumPlane planeId, glm::vec3& direction, glm::vec3& up) const
{
	auto& corners = m_Corners.Corners;
	switch (planeId)
	{
	case FrustumPlane::Left:
		up = glm::normalize(corners[(int)FrustumCorner::FarTopLeft]
			+ corners[(int)FrustumCorner::FarBottomLeft]
			- corners[(int)FrustumCorner::NearTopLeft]
			- corners[(int)FrustumCorner::NearBottomLeft]);
		direction = glm::normalize(corners[(int)FrustumCorner::NearTopLeft] - corners[(int)FrustumCorner::NearBottomLeft]);
		break;
	case FrustumPlane::Right:
		up = glm::normalize(corners[(int)FrustumCorner::FarTopRight]
			+ corners[(int)FrustumCorner::FarBottomRight]
			- corners[(int)FrustumCorner::NearTopRight]
			- corners[(int)FrustumCorner::NearBottomRight]);
		direction = glm::normalize(corners[(int)FrustumCorner::NearBottomRight] - corners[(int)FrustumCorner::NearTopRight]);
		break;
	case FrustumPlane::Bottom:
		up = glm::normalize(corners[(int)FrustumCorner::FarBottomLeft]
			+ corners[(int)FrustumCorner::FarBottomRight]
			- corners[(int)FrustumCorner::NearBottomLeft]
			- corners[(int)FrustumCorner::NearBottomRight]);
		direction = glm::normalize(corners[(int)FrustumCorner::NearBottomLeft] - corners[(int)FrustumCorner::NearBottomRight]);
		break;
	case FrustumPlane::Top:
		up = glm::normalize(corners[(int)FrustumCorner::FarTopLeft]
			+ corners[(int)FrustumCorner::FarTopRight]
			- corners[(int)FrustumCorner::NearTopLeft]
			- corners[(int)FrustumCorner::NearTopRight]);
		direction = glm::normalize(corners[(int)FrustumCorner::NearTopRight] - corners[(int)FrustumCorner::NearTopLeft]);
		break;
	case FrustumPlane::Near:
		up = glm::normalize(corners[(int)FrustumCorner::NearTopLeft] - corners[(int)FrustumCorner::NearBottomLeft]);
		direction = glm::normalize(corners[(int)FrustumCorner::NearTopRight] - corners[(int)FrustumCorner::NearTopLeft]);
		break;
	case FrustumPlane::Far:
		up = glm::normalize(corners[(int)FrustumCorner::FarTopLeft] - corners[(int)FrustumCorner::FarBottomLeft]);
		direction = glm::normalize(corners[(int)FrustumCorner::FarTopLeft] - corners[(int)FrustumCorner::FarTopRight]);
		break;
	}
}

inline float GetSidePlaneUpExtent(float n, float f, float s)
{
	return sqrtf(n * n + s * s) * (f / n - 1.0f);
}

float BoundingFrustum::GetPlaneUpExtent(FrustumPlane planeId) const
{
	if (m_ProjectionType == Graphics::ProjectionType::Perspective)
	{
		auto& pProjection = m_Projection.Projection.Perspective;

		float n = pProjection.NearPlaneDistance;
		float f = pProjection.FarPlaneDistance;

		switch (planeId)
		{
		case FrustumPlane::Left:
			return GetSidePlaneUpExtent(n, f, pProjection.Left);
		case FrustumPlane::Right:
			return GetSidePlaneUpExtent(n, f, pProjection.Right);
		case FrustumPlane::Bottom:
			return GetSidePlaneUpExtent(n, f, pProjection.Bottom);
		case FrustumPlane::Top:
			return GetSidePlaneUpExtent(n, f, pProjection.Top);
		case FrustumPlane::Near:
			return (pProjection.Top - pProjection.Bottom);
		case FrustumPlane::Far:
			return (pProjection.Top - pProjection.Bottom) * f / n;
		}
	}
	else if (m_ProjectionType == Graphics::ProjectionType::Orthographic)
	{
		auto& oProjection = m_Projection.Projection.Orthographic;

		switch (planeId)
		{
		case FrustumPlane::Left:
		case FrustumPlane::Right:
		case FrustumPlane::Bottom:
		case FrustumPlane::Top:
			return oProjection.FarPlaneDistance - oProjection.NearPlaneDistance;
		case FrustumPlane::Near:
		case FrustumPlane::Far:
			return oProjection.Top - oProjection.Bottom;
		}
	}
	return 0.0f;
}

glm::vec2 BoundingFrustum::GetNearPlaneSize() const
{
	auto& corners = m_Corners.Corners;
	glm::vec2 size;
	size.x = glm::length(corners[(int)FrustumCorner::NearBottomLeft] - corners[(int)FrustumCorner::NearBottomRight]);
	size.y = glm::length(corners[(int)FrustumCorner::NearBottomLeft] - corners[(int)FrustumCorner::NearTopLeft]);
	return size;
}

void BoundingFrustum::GetNearPlaneOrientation(glm::vec3& direction, glm::vec3& up, glm::vec3& right) const
{
	auto& corners = m_Corners.Corners;
	up = glm::normalize(corners[(int)FrustumCorner::NearTopLeft]
		- corners[(int)FrustumCorner::NearBottomLeft]);
	right = glm::normalize(corners[(int)FrustumCorner::NearBottomRight]
		- corners[(int)FrustumCorner::NearBottomLeft]);
	direction = glm::cross(up, right);
}

void BoundingFrustum::GetSidePointsAlongDirection(const glm::vec3& direction,
	glm::vec3& min, glm::vec3& max) const
{
	auto& corners = m_Corners.Corners;
	unsigned minIndex = 0;
	unsigned maxIndex = 0;
	float dotMin = glm::dot(direction, corners[0]);
	float dotMax = dotMin;
	for (unsigned i = 1; i < c_CountFrustumCorners; i++)
	{
		float currentDot = glm::dot(direction, corners[i]);
		if (currentDot < dotMin)
		{
			minIndex = (unsigned)i;
			dotMin = currentDot;
		}
		else if (currentDot > dotMax)
		{
			maxIndex = (unsigned)i;
			dotMax = currentDot;
		}
	}
	min = corners[minIndex];
	max = corners[maxIndex];
}