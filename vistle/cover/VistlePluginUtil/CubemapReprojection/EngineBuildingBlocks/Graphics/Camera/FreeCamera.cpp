// EngineBuildingBlocks/Graphics/Camera/FreeCamera.cpp

#include <EngineBuildingBlocks/Graphics/Camera/FreeCamera.h>

#include <EngineBuildingBlocks/SystemTime.h>
#include <EngineBuildingBlocks/ErrorHandling.h>
#include <EngineBuildingBlocks/Input/KeyHandler.h>
#include <EngineBuildingBlocks/Input/MouseHandler.h>

#include <cmath>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;

FreeCamera::FreeCamera(SceneNodeHandler* sceneNodeHandler,
	Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler)
	: Camera(sceneNodeHandler)
{
	Initialize(keyHandler, mouseHandler);
}

FreeCamera::FreeCamera(SceneNodeHandler* sceneNodeHandler, 
	Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler, const glm::vec3& position, const glm::vec3& direction,
	float fovY, float aspectRatio, float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval)
	: Camera(sceneNodeHandler)
{
	Camera::SetLocation(position, direction);
	Camera::SetPerspectiveProjection(fovY, aspectRatio, nearPlaneDistance, farPlaneDistance,
		isProjectingTo_0_1_Interval);

	Initialize(keyHandler, mouseHandler);
}

FreeCamera::FreeCamera(SceneNodeHandler* sceneNodeHandler, 
	Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler, const glm::vec3& position, const glm::vec3& direction,
	float left, float right, float bottom, float top, float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval)
	: Camera(sceneNodeHandler)
{
	Camera::SetLocation(position, direction);
	Camera::SetPerspectiveProjection(left, right, bottom, top, nearPlaneDistance, farPlaneDistance,
		isProjectingTo_0_1_Interval);

	Initialize(keyHandler, mouseHandler);
}

void FreeCamera::Initialize(Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler)
{
	m_UpdateVector = m_SpeedTime = glm::vec3(0.0f);
	m_IsFrozen = false;

	m_MaxSpeed = 2.0f;
	m_MaxSpeedAccelleration = 10.0f;
	m_AccellerationTime = 0.2f;
	m_MouseSensitivity = 1.0f;
	m_PixelMultiplier = 0.002f;

	// Registering for key events.
	if (keyHandler != nullptr)
	{
		auto eventClassIds = keyHandler->RegisterTimedKeyEventListener(
			{
				"FreeCamera.MoveForward",
				"FreeCamera.MoveBackward",
				"FreeCamera.MoveUp",
				"FreeCamera.MoveDown",
				"FreeCamera.MoveLeft",
				"FreeCamera.MoveRight",
				"FreeCamera.IncreaseMoveSpeed",
				"FreeCamera.DecreaseMoveSpeed"
			}, this);

		KeyEventName_MoveForward_ECI = eventClassIds[0];
		KeyEventName_MoveBackward_ECI = eventClassIds[1];
		KeyEventName_MoveUp_ECI = eventClassIds[2];
		KeyEventName_MoveDown_ECI = eventClassIds[3];
		KeyEventName_MoveLeft_ECI = eventClassIds[4];
		KeyEventName_MoveRight_ECI = eventClassIds[5];
		KeyEventName_IncreaseMoveSpeed_ECI = eventClassIds[6];
		KeyEventName_DecreaseMoveSpeed_ECI = eventClassIds[7];
	}

	// Registering for mouse events.
	if (mouseHandler != nullptr)
	{
		MouseEventName_Rotate_ECI
			= mouseHandler->RegisterMouseDragEventListener("FreeCamera.Rotate", this);
	}
}

float FreeCamera::GetMaxSpeed() const
{
	return m_MaxSpeed;
}

void FreeCamera::SetMaxSpeed(float maxSpeed)
{
	this->m_MaxSpeed = maxSpeed;
}

float FreeCamera::GetMaxSpeedAccelleration() const
{
	return m_MaxSpeedAccelleration;
}

void FreeCamera::SetMaxSpeedAccelleration(float acceleration)
{
	this->m_MaxSpeedAccelleration = acceleration;
}

float FreeCamera::GetAccellerationTime() const
{
	return m_AccellerationTime;
}

void FreeCamera::SetAccellerationTime(float time)
{
	this->m_AccellerationTime = time;
}

float FreeCamera::GetMouseSensitivity() const
{
	return m_MouseSensitivity;
}

void FreeCamera::SetMouseSensitivity(float sensitivity)
{
	this->m_MouseSensitivity = sensitivity;
}

void FreeCamera::SetFrozen(bool isFrozen)
{
	this->m_IsFrozen = isFrozen;
}

bool FreeCamera::HandleEvent(const Event* _event)
{
	if (m_IsFrozen)
	{
		return false;
	}

	unsigned eventClassId = _event->ClassId;
	
	if (eventClassId == KeyEventName_MoveForward_ECI)
	{
		m_UpdateVector.x += 1.0f;
	}
	else if (eventClassId == KeyEventName_MoveBackward_ECI)
	{
		m_UpdateVector.x -= 1.0f;
	}
	else if (eventClassId == KeyEventName_MoveUp_ECI)
	{
		m_UpdateVector.y += 1.0f;
	}
	else if (eventClassId == KeyEventName_MoveDown_ECI)
	{
		m_UpdateVector.y -= 1.0f;
	}
	else if (eventClassId == KeyEventName_MoveLeft_ECI)
	{
		m_UpdateVector.z -= 1.0f;
	}
	else if (eventClassId == KeyEventName_MoveRight_ECI)
	{
		m_UpdateVector.z += 1.0f;
	}
	else if (eventClassId == KeyEventName_IncreaseMoveSpeed_ECI)
	{
		float dt = static_cast<float>(Input::ToKeyEvent(_event).SystemTime->GetElapsedTime());
		m_MaxSpeed += m_MaxSpeedAccelleration * dt;
	}
	else if (eventClassId == KeyEventName_DecreaseMoveSpeed_ECI)
	{
		float dt = static_cast<float>(Input::ToKeyEvent(_event).SystemTime->GetElapsedTime());
		m_MaxSpeed -= m_MaxSpeedAccelleration * dt;
		if (m_MaxSpeed < 0.0f)
		{
			m_MaxSpeed = 0.0f;
		}
	}
	else if (eventClassId == MouseEventName_Rotate_ECI)
	{
		auto worldTransformation = GetWorldTransformation();
		auto& direction = worldTransformation.A[0];
		auto& right = worldTransformation.A[2];

		auto& mouseDragEvent = EngineBuildingBlocks::Input::ToMouseDragEvent(_event);
		glm::vec2 difference;
		mouseDragEvent.GetDifference(difference.x, difference.y);

		float wUpAngle = -difference.x * m_MouseSensitivity * m_PixelMultiplier;
		float rightAngle = -difference.y * m_MouseSensitivity * m_PixelMultiplier;

		glm::mat4 rotation = glm::rotate(wUpAngle, SceneNodeHandler::c_WorldUp)
			* glm::rotate(rightAngle, right);
		glm::vec4 newDirection = rotation * glm::vec4(direction, 0.0f);

		// Preventing camera flip.
		{
			const float yLimit = 0.99f;

			newDirection = glm::normalize(newDirection);
			float currentXZLengthSqr = newDirection.x * newDirection.x + newDirection.z * newDirection.z;
			float requiredXZLengthSqr = 1.0f - yLimit * yLimit;
			float ratio = sqrtf(requiredXZLengthSqr / currentXZLengthSqr);
			if (newDirection.y < -yLimit)
			{
				newDirection.y = -yLimit;
				newDirection.x *= ratio;
				newDirection.z *= ratio;
			}
			if (newDirection.y > yLimit)
			{
				newDirection.y = yLimit;
				newDirection.x *= ratio;
				newDirection.z *= ratio;
			}
		}

		SetDirection(glm::vec3(newDirection.x, newDirection.y, newDirection.z));
	}
	else
	{
		RaiseException("Unknown event was received.");
	}

	return true;
}

float FreeCamera::ChangeSpeedTime(float& speedTime, float update, float updateDT)
{
	// Modifying speed time using the update data.
	if (update == 0.0f)
	{
		float oldSpeedTime = speedTime;
		speedTime -= (speedTime >= 0.0f ? 1.0f : -1.0f) * updateDT;
		if (speedTime * oldSpeedTime <= 0.0f)
		{
			speedTime = 0.0f;
		}
	}
	else
	{
		if (speedTime == 0.0f)
		{
			speedTime += update * updateDT;
		}
		else
		{
			speedTime += (3.0f * update - (speedTime >= 0.0f ? 1.0f : -1.0f)) / 2.0f * updateDT;
		}
	}

	// Clamping speed time.
	if (speedTime < -1.0f)
	{
		speedTime = -1.0f;
	}
	else if (speedTime > 1.0f)
	{
		speedTime = 1.0f;
	}

	// Returning signed smoothstep for the speed.
	if (speedTime < 0.0f)
	{
		float x = -speedTime;
		return -(x * x * (3.0f - 2.0f * x));
	}
	float x = speedTime;
	return x * x * (3.0f - 2.0f * x);
}

void FreeCamera::Update(const SystemTime& systemTime)
{
	float dt = static_cast<float>(systemTime.GetElapsedTime());
	float speedUpdateTime = (dt / m_AccellerationTime);

	float speedX = ChangeSpeedTime(m_SpeedTime.x, m_UpdateVector.x, speedUpdateTime) * m_MaxSpeed;
	float speedY = ChangeSpeedTime(m_SpeedTime.y, m_UpdateVector.y, speedUpdateTime) * m_MaxSpeed;
	float speedZ = ChangeSpeedTime(m_SpeedTime.z, m_UpdateVector.z, speedUpdateTime) * m_MaxSpeed;

	auto worldTransformation = GetWorldTransformation();
	auto& position = worldTransformation.Position;
	auto& direction = worldTransformation.A[0];
	auto& up = worldTransformation.A[1];
	auto& right = worldTransformation.A[2];

	if (speedX == 0.0f && speedY == 0.0f && speedZ == 0.0f)
	{
		return;
	}

	position += (speedX * direction + speedY * up + speedZ * right) * dt;

	SetPosition(position);

	m_UpdateVector = glm::vec3(0.0f);
}