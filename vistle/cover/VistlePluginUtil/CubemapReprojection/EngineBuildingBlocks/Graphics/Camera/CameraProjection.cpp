// EngineBuildingBlocks/Graphics/Camera/CameraProjection.cpp

#include <EngineBuildingBlocks/Graphics/Camera/CameraProjection.h>

#include <cmath>

using namespace EngineBuildingBlocks::Graphics;

void PerspectiveOrthographicProjection::Set(float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval,
	float fovY, float aspectRatio)
{
	this->NearPlaneDistance = nearPlaneDistance;
	this->FarPlaneDistance = farPlaneDistance;
	this->IsProjectingTo_0_1_Interval = isProjectingTo_0_1_Interval;

	float height2 = tanf(fovY * 0.5f) * nearPlaneDistance;
	float width2 = height2 * aspectRatio;

	this->Left = -width2;
	this->Right = width2;
	this->Bottom = -height2;
	this->Top = height2;
}

void PerspectiveOrthographicProjection::Set(float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval,
	float left, float right, float bottom, float top)
{
	this->NearPlaneDistance = nearPlaneDistance;
	this->FarPlaneDistance = farPlaneDistance;
	this->IsProjectingTo_0_1_Interval = isProjectingTo_0_1_Interval;
	this->Left = left;
	this->Right = right;
	this->Bottom = bottom;
	this->Top = top;
}

float PerspectiveOrthographicProjection::GetAspectRatio() const
{
	return (Right - Left) / (Top - Bottom);
}

float PerspectiveOrthographicProjection::GetNearPlaneWidth() const
{
	return (Right - Left);
}

float PerspectiveOrthographicProjection::GetNearPlaneHeight() const
{
	return (Top - Bottom);
}

float PerspectiveOrthographicProjection::GetFarPlaneWidth() const
{
	return (Right - Left) * FarPlaneDistance / NearPlaneDistance;
}

float PerspectiveOrthographicProjection::GetFarPlaneHeight() const
{
	return (Top - Bottom) * FarPlaneDistance / NearPlaneDistance;
}

void PerspectiveOrthographicProjection::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, NearPlaneDistance);
	Core::SerializeSB(bytes, FarPlaneDistance);
	Core::SerializeSB(bytes, IsProjectingTo_0_1_Interval);
	Core::SerializeSB(bytes, Left);
	Core::SerializeSB(bytes, Right);
	Core::SerializeSB(bytes, Bottom);
	Core::SerializeSB(bytes, Top);
}

void PerspectiveOrthographicProjection::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, NearPlaneDistance);
	Core::DeserializeSB(bytes, FarPlaneDistance);
	Core::DeserializeSB(bytes, IsProjectingTo_0_1_Interval);
	Core::DeserializeSB(bytes, Left);
	Core::DeserializeSB(bytes, Right);
	Core::DeserializeSB(bytes, Bottom);
	Core::DeserializeSB(bytes, Top);
}

float CameraProjection::Projection::Perspective::GetFovX() const
{
	return atanf(-Left / NearPlaneDistance) + atanf(Right / NearPlaneDistance);
}

float CameraProjection::Projection::Perspective::GetFovY() const
{
	return atanf(-Bottom / NearPlaneDistance) + atanf(Top / NearPlaneDistance);
}

float CameraProjection::Projection::Perspective::GetLinearDepth(float perspectiveDepth) const
{
	float n = NearPlaneDistance;
	float f = FarPlaneDistance;
	if(IsProjectingTo_0_1_Interval) return n * f / ((f - n) * perspectiveDepth - f);
	return 2.0f * n * f / ((f - n) * perspectiveDepth - (n + f));
}

float CameraProjection::Projection::Perspective::GetPerspectiveDepth(float linearDepth) const
{
	float n = NearPlaneDistance;
	float f = FarPlaneDistance;
	if (IsProjectingTo_0_1_Interval) return f / (f - n) * (n / linearDepth + 1);
	return ((2.0f * n * f) / linearDepth + n + f) / (f - n);
}

float CameraProjection::GetAspectRatio() const
{
	switch (Type)
	{
	case ProjectionType::Perspective:
		return Projection.Perspective.GetAspectRatio();
	case ProjectionType::Orthographic:
		return Projection.Orthographic.GetAspectRatio();
	}
	return 0.0f;
}

void CameraProjection::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Type);
	switch (Type)
	{
	case ProjectionType::Perspective: Core::SerializeSB(bytes, Projection.Perspective); break;
	case ProjectionType::Orthographic: Core::SerializeSB(bytes, Projection.Orthographic); break;
	}
}

void CameraProjection::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Type);
	switch (Type)
	{
	case ProjectionType::Perspective: Core::DeserializeSB(bytes, Projection.Perspective); break;
	case ProjectionType::Orthographic: Core::DeserializeSB(bytes, Projection.Orthographic); break;
	}
}

// Note that currently only the OpenGL-projections are supported!

bool CameraProjection::CreateFromMatrix(const glm::mat4& m)
{
	// Trying to interpret as a perspective projection.
	{
		auto zero
			= abs(m[0][1]) + abs(m[0][2]) + abs(m[0][3])
			+ abs(m[1][0]) + abs(m[1][2]) + abs(m[1][3])
			+ abs(m[3][0]) + abs(m[3][1]) + abs(m[3][3]);
		if (zero == 0.0f && m[2][3] == -1.0f)
		{
			float c1 = m[0][0], c2 = m[1][1], c3 = m[2][0], c4 = m[2][1], c5 = m[2][2], c6 = m[3][2];
			float n = c6 / (c5 - 1);
			float f = c6 / (c5 + 1);
			float bx = n / c1;
			float by = n / c2;
			float l = bx * (c3 - 1);
			float r = bx * (c3 + 1);
			float b = by * (c4 - 1);
			float t = by * (c4 + 1);
			Projection.Perspective.NearPlaneDistance = n;
			Projection.Perspective.FarPlaneDistance = f;
			Projection.Perspective.IsProjectingTo_0_1_Interval = false;
			Projection.Perspective.Left = l;
			Projection.Perspective.Right = r;
			Projection.Perspective.Bottom = b;
			Projection.Perspective.Top = t;
			Type = ProjectionType::Perspective;
			return true;
		}
	}

	// Trying to interpret as an orthographic projection.
	{
		// TODO: implement this if needed...
	}

	return false;
}

void CameraProjection::CreatePerspecive(float aspectRatio, float fovY, float near, float far,
	bool isProjectingTo_0_1)
{
	float h2 = near * tan(fovY * 0.5f);
	float w2 = aspectRatio * h2;

	Projection.Perspective.NearPlaneDistance = near;
	Projection.Perspective.FarPlaneDistance = far;
	Projection.Perspective.IsProjectingTo_0_1_Interval = isProjectingTo_0_1;
	Projection.Perspective.Left = -w2;
	Projection.Perspective.Right = w2;
	Projection.Perspective.Bottom = -h2;
	Projection.Perspective.Top = h2;
	Type = ProjectionType::Perspective;
}