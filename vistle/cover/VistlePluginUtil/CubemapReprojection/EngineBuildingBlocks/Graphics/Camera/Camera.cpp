// EngineBuildingBlocks/Graphics/Camera/Camera.cpp

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>

#include <EngineBuildingBlocks/Math/Matrix.h>

using namespace EngineBuildingBlocks::Graphics;

Camera::Camera(EngineBuildingBlocks::SceneNodeHandler* sceneNodeHandler)
	: m_SceneNode(sceneNodeHandler, false, 0)
	, m_IsViewProjectionMatrixRecomputationNeeded(true)
{
	SetDefaultPerspectiveProjection(true);
}

const EngineBuildingBlocks::WrappedSceneNode& Camera::GetWrappedSceneNode() const
{
	return m_SceneNode;
}

EngineBuildingBlocks::WrappedSceneNode& Camera::GetWrappedSceneNode()
{
	return m_SceneNode;
}

EngineBuildingBlocks::SceneNodeHandler* Camera::GetSceneNodeHandler()
{
	return m_SceneNode.GetSceneNodeHandler();
}

void Camera::Set(Camera& camera)
{
	auto transformation = camera.GetWorldTransformation();
	SetPosition(transformation.Position);
	SetOrientation(transformation.A);
	SetProjection(camera.GetProjection());
}

///////////////////////////////////////////////////////////////////////////////////////////

void Camera::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_Projection);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_SceneNode.GetLocalOrientation()));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_SceneNode.GetLocalPosition()));
}

void Camera::DeserializeSB(const unsigned char*& bytes)
{
	CameraProjection projection;
	Core::DeserializeSB(bytes, projection);
	SetProjection(projection);

	glm::mat3 orientation(glm::uninitialize);
	glm::vec3 position(glm::uninitialize);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(orientation));
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(position));

	m_SceneNode.SetLocalOrientation(orientation);
	m_SceneNode.SetLocalPosition(position);

	m_IsViewProjectionMatrixRecomputationNeeded = true;
}

///////////////////////////////////////////////////////////////////////////////////////////

void Camera::SetFromViewMatrix(glm::mat4& viewMatrix)
{
	glm::mat3 a(viewMatrix);
	auto aInv = glm::transpose(a);

	glm::mat4 transformation(glm::uninitialize);
	transformation[0] = glm::vec4(glm::normalize(-aInv[2]), 0.0f);
	transformation[1] = glm::vec4(glm::normalize(aInv[1]), 0.0f);
	transformation[2] = glm::vec4(glm::normalize(aInv[0]), 0.0f);
	transformation[3] = glm::vec4(-aInv * glm::vec3(viewMatrix[3]), 1.0f);

	m_ViewMatrix = viewMatrix;
	m_SceneNode.SetLocation(transformation);
	m_SceneNode.SetTransformationUpToDateForUser();
	m_IsViewProjectionMatrixRecomputationNeeded = true;
}

void Camera::SetViewMatrix(const EngineBuildingBlocks::ScaledTransformation& worldTransformation, glm::mat4& viewMatrix)
{
	auto& position = worldTransformation.Position;

	// Making the base orthonormal.
	auto direction = glm::normalize(worldTransformation.A[0]);
	auto right = glm::normalize(glm::cross(direction, worldTransformation.A[1]));
	auto up = glm::cross(right, direction);

	// Computing view matrix.
	viewMatrix[0][0] = right.x;
	viewMatrix[1][0] = right.y;
	viewMatrix[2][0] = right.z;
	viewMatrix[0][1] = up.x;
	viewMatrix[1][1] = up.y;
	viewMatrix[2][1] = up.z;
	viewMatrix[0][2] = -direction.x;
	viewMatrix[1][2] = -direction.y;
	viewMatrix[2][2] = -direction.z;
	viewMatrix[3][0] = -glm::dot(right, position);
	viewMatrix[3][1] = -glm::dot(up, position);
	viewMatrix[3][2] = glm::dot(direction, position);
}

void Camera::UpdateViewMatrix()
{
	if (m_SceneNode.IsTransformationDirtyForUser())
	{
		SetViewMatrix(m_SceneNode.GetWorldTransformation(), m_ViewMatrix);

		m_SceneNode.SetTransformationUpToDateForUser();

		m_IsViewProjectionMatrixRecomputationNeeded = true;
	}
}

void Camera::UpdateViewProjectionMatrix()
{
	UpdateViewMatrix();

	if (m_IsViewProjectionMatrixRecomputationNeeded)
	{
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_Frustum.SetData(m_ViewProjectionMatrix, m_Projection);

		m_IsViewProjectionMatrixRecomputationNeeded = false;
	}
}

const glm::mat4& Camera::GetViewMatrix()
{
	UpdateViewMatrix();
	return m_ViewMatrix;
}

const glm::mat4& Camera::GetProjectionMatrix()
{
	return m_ProjectionMatrix;
}

const glm::mat4& Camera::GetViewProjectionMatrix()
{
	UpdateViewProjectionMatrix();
	return m_ViewProjectionMatrix;
}

glm::mat4 Camera::GetViewMatrixInverse()
{
	UpdateViewMatrix();
	return glm::transpose(m_ViewMatrix);
}

glm::mat4 Camera::GetProjectionMatrixInverse()
{
	return glm::inverse(m_ProjectionMatrix);
}

glm::mat4 Camera::GetViewProjectionMatrixInverse()
{
	UpdateViewProjectionMatrix();
	return glm::inverse(m_ViewProjectionMatrix);
}

const EngineBuildingBlocks::Math::BoundingFrustum& Camera::GetViewFrustum()
{
	UpdateViewProjectionMatrix();
	return m_Frustum;
}

///////////////////////////////////////////////////////////////////////////////////////////

CameraData Camera::GetData() const
{
	return { GetProjection(), { GetLocalOrientation(), GetLocalPosition() } };
}

void Camera::SetData(const CameraData& data)
{
	SetProjection(data.Projection);
	SetLocalTransformation(data.LocalTransformation);
}

const CameraProjection& Camera::GetProjection() const
{
	return m_Projection;
}

void Camera::SetProjection(const CameraProjection& projection)
{
	switch (projection.Type)
	{
	case ProjectionType::Perspective:
	{
		auto& pProj = projection.Projection.Perspective;
		SetPerspectiveProjection(pProj.Left, pProj.Right, pProj.Bottom, pProj.Top,
			pProj.NearPlaneDistance, pProj.FarPlaneDistance, pProj.IsProjectingTo_0_1_Interval);
		break;
	}
	case ProjectionType::Orthographic:
	{
		auto& oProj = projection.Projection.Orthographic;
		SetOrthographicProjection(oProj.Left, oProj.Right, oProj.Bottom, oProj.Top,
			oProj.NearPlaneDistance, oProj.FarPlaneDistance, oProj.IsProjectingTo_0_1_Interval);
		break;
	}
	}
}

ProjectionType Camera::GetProjectionType() const
{
	return m_Projection.Type;
}

bool Camera::IsProjectingTo_0_1_Interval() const
{
	switch (m_Projection.Type)
	{
		case ProjectionType::Perspective: return m_Projection.Projection.Perspective.IsProjectingTo_0_1_Interval;
		case ProjectionType::Orthographic: return m_Projection.Projection.Orthographic.IsProjectingTo_0_1_Interval;
	}
	return false;
}

EngineBuildingBlocks::Math::BoundingFrustum Camera::GetProjectionOnlyFrustum() const
{
	return EngineBuildingBlocks::Math::BoundingFrustum(m_ProjectionMatrix, m_Projection);
}

void Camera::SetDefaultPerspectiveProjection(bool isProjectingTo_0_1_Interval)
{
	SetPerspectiveProjection(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f, isProjectingTo_0_1_Interval);
}

void Camera::SetPerspectiveProjection(float fovY, float aspectRatio,
	float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval)
{
	m_Projection.Projection.Perspective.Set(nearPlaneDistance, farPlaneDistance, isProjectingTo_0_1_Interval,
		fovY, aspectRatio);
	m_Projection.Type = ProjectionType::Perspective;

	auto& p = m_Projection.Projection.Perspective;
	SetPerspectiveProjectionMatrix(p.Left, p.Right, p.Bottom, p.Top,
		nearPlaneDistance, farPlaneDistance, isProjectingTo_0_1_Interval);
}

void Camera::SetPerspectiveProjection(float left, float right, float bottom, float top,
	float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval)
{
	m_Projection.Projection.Perspective.Set(nearPlaneDistance, farPlaneDistance, isProjectingTo_0_1_Interval,
		left, right, bottom, top);
	m_Projection.Type = ProjectionType::Perspective;

	SetPerspectiveProjectionMatrix(left, right, bottom, top,
		nearPlaneDistance, farPlaneDistance, isProjectingTo_0_1_Interval);
}

void Camera::SetPerspectiveProjectionMatrix(float left, float right, float bottom, float top,
	float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval)
{
	m_ProjectionMatrix = glm::frustum(left, right, bottom, top, nearPlaneDistance, farPlaneDistance);
	m_IsViewProjectionMatrixRecomputationNeeded = true;

	if (isProjectingTo_0_1_Interval)
	{
		float m = 1.0f / (nearPlaneDistance - farPlaneDistance);
		m_ProjectionMatrix[2][2] = farPlaneDistance * m;
		m_ProjectionMatrix[3][2] = nearPlaneDistance * farPlaneDistance * m;
	}
}

void Camera::SetOrthographicProjection(float left, float right, float bottom, float top,
	float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval)
{
	m_Projection.Projection.Orthographic.Set(nearPlaneDistance, farPlaneDistance, isProjectingTo_0_1_Interval,
		left, right, bottom, top);
	m_Projection.Type = ProjectionType::Orthographic;

	m_ProjectionMatrix = glm::ortho(left, right, bottom, top, nearPlaneDistance, farPlaneDistance);
	m_IsViewProjectionMatrixRecomputationNeeded = true;

	if (isProjectingTo_0_1_Interval)
	{
		float m = 1.0f / (nearPlaneDistance - farPlaneDistance);
		m_ProjectionMatrix[2][2] = m;
		m_ProjectionMatrix[3][2] = nearPlaneDistance * m;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

bool Camera::HasSameLocationAndProjection(Camera& other, float epsilon)
{
	return IsEqual(GetViewProjectionMatrix(), other.GetViewProjectionMatrix(), epsilon);
}

void Camera::SetLocation(Camera& other)
{
	assert(m_SceneNode.GetParentSceneNodeIndex() == other.m_SceneNode.GetParentSceneNodeIndex());
	
	SetLocalTransformation(other.GetLocalTransformation());
}

void Camera::SetLocationAndProjection(Camera& other)
{
	SetLocation(other);
	SetProjection(other.GetProjection());
}

///////////////////////////////////////////////////////////////////////////////////////////

const glm::vec3& Camera::GetLocalPosition() const
{
	return m_SceneNode.GetLocalPosition();
}

void Camera::SetLocalPosition(const glm::vec3& localPosition)
{
	m_SceneNode.SetLocalPosition(localPosition);
}

const EngineBuildingBlocks::RigidTransformation& Camera::GetLocalTransformation() const
{
	return m_SceneNode.GetLocalTransformation();
}

const glm::mat3& Camera::GetLocalOrientation() const
{
	return m_SceneNode.GetLocalOrientation();
}

void Camera::SetLocalOrientation(const glm::mat3& localOrientation)
{
	m_SceneNode.SetLocalOrientation(localOrientation);
}

const glm::vec3& Camera::GetScaler() const
{
	return m_SceneNode.GetScaler();
}

void Camera::SetScaler(const glm::vec3& scaler)
{
	m_SceneNode.SetScaler(scaler);
}

EngineBuildingBlocks::ScaledTransformation Camera::GetWorldTransformation()
{
	return m_SceneNode.GetWorldTransformation();
}

glm::mat3 Camera::GetWorldOrientation()
{
	return m_SceneNode.GetWorldOrientation();
}

EngineBuildingBlocks::ScaledTransformation Camera::GetInverseWorldTransformation()
{
	return m_SceneNode.GetInverseWorldTransformation();
}

const EngineBuildingBlocks::ScaledTransformation& Camera::GetScaledWorldTransformation()
{
	return m_SceneNode.GetScaledWorldTransformation();
}

const EngineBuildingBlocks::ScaledTransformation& Camera::GetInverseScaledWorldTransformation()
{
	return m_SceneNode.GetInverseScaledWorldTransformation();
}

const glm::vec3& Camera::GetPosition()
{
	return m_SceneNode.GetPosition();
}

void Camera::SetPosition(const glm::vec3& position)
{
	m_SceneNode.SetPosition(position);
}

glm::vec3 Camera::GetDirection()
{
	return m_SceneNode.GetDirection();
}

glm::vec3 Camera::GetUp()
{
	return m_SceneNode.GetUp();
}

glm::vec3 Camera::GetRight()
{
	return m_SceneNode.GetRight();
}

glm::mat3 Camera::GetOrientation() const
{
	return m_SceneNode.GetOrientation();
}

glm::quat Camera::GetOrientationQuaternion() const
{
	return m_SceneNode.GetOrientationQuaternion();
}

glm::mat3 Camera::GetViewOrientation() const
{
	auto orientation = m_SceneNode.GetOrientation();
	return glm::mat3(orientation[2], orientation[1], -orientation[0]);
}

void Camera::SetOrientation(const glm::mat3& orientation)
{
	m_SceneNode.SetOrientation(orientation);
}

void Camera::SetOrientation(const glm::quat& orientation)
{
	m_SceneNode.SetOrientation(orientation);
}

void Camera::SetViewOrientation(const glm::mat3& orientation)
{
	SetOrientation(glm::mat3(-orientation[2], orientation[1], orientation[0]));
}

void Camera::SetDirectionUp(const glm::vec3& direction, const glm::vec3& up)
{
	m_SceneNode.SetDirectionUp(direction, up);
}

void Camera::SetDirection(const glm::vec3& direction)
{
	m_SceneNode.SetDirection(direction);
}

void Camera::SetLocalTransformation(const EngineBuildingBlocks::RigidTransformation& transformation)
{
	m_SceneNode.SetLocalTransformation(transformation);
}

void Camera::SetLocalTransformation(const EngineBuildingBlocks::ScaledTransformation& transformation)
{
	m_SceneNode.SetLocalTransformation(transformation);
}

void Camera::SetLocation(const EngineBuildingBlocks::ScaledTransformation& transformation)
{
	m_SceneNode.SetLocation(transformation);
}

void Camera::SetLocation(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up)
{
	m_SceneNode.SetLocation(position, direction, up);
}

void Camera::SetLocation(const glm::vec3& position, const glm::vec3& direction)
{
	m_SceneNode.SetLocation(position, direction);
}

glm::vec3 Camera::GetLookAt()
{
	return m_SceneNode.GetLookAt();
}

void Camera::LookAt(const glm::vec3& lookAtPosition)
{
	m_SceneNode.LookAt(lookAtPosition);
}