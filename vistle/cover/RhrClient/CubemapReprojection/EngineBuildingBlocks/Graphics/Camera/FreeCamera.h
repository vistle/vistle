// EngineBuildingBlocks/Graphics/Camera/FreeCamera.h

#ifndef _ENGINEBUILDINGBLOCKS_FREECAMERA_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_FREECAMERA_H_INCLUDED_

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>

#include <EngineBuildingBlocks/EventHandling.h>

namespace EngineBuildingBlocks
{
	class SystemTime;

	namespace Input
	{
		class KeyHandler;
		class MouseHandler;
	}

	namespace Graphics
	{
		class FreeCamera
			: public Camera
			, public IEventListener
		{
		protected:

			bool m_IsFrozen;
			glm::vec3 m_UpdateVector;
			glm::vec3 m_SpeedTime;

			float m_MaxSpeed;
			float m_MaxSpeedAccelleration;
			float m_AccellerationTime;
			float m_PixelMultiplier;
			float m_MouseSensitivity;

			void Initialize(Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler);

			float ChangeSpeedTime(float& speedTime, float updateTime, float updateDT);

		public:

			unsigned KeyEventName_MoveForward_ECI;
			unsigned KeyEventName_MoveBackward_ECI;
			unsigned KeyEventName_MoveUp_ECI;
			unsigned KeyEventName_MoveDown_ECI;
			unsigned KeyEventName_MoveLeft_ECI;
			unsigned KeyEventName_MoveRight_ECI;
			unsigned KeyEventName_IncreaseMoveSpeed_ECI;
			unsigned KeyEventName_DecreaseMoveSpeed_ECI;

			unsigned MouseEventName_Rotate_ECI;

			FreeCamera(SceneNodeHandler* sceneNodeHandler, Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler);
			FreeCamera(SceneNodeHandler* sceneNodeHandler, Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler, const glm::vec3& position, const glm::vec3& direction,
				float fovY, float aspectRatio, float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval);
			FreeCamera(SceneNodeHandler* sceneNodeHandler, Input::KeyHandler* keyHandler, Input::MouseHandler* mouseHandler, const glm::vec3& position, const glm::vec3& direction,
				float left, float right, float bottom, float top, float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval);

			float GetMaxSpeed() const;
			void SetMaxSpeed(float maxSpeed);

			float GetMaxSpeedAccelleration() const;
			void SetMaxSpeedAccelleration(float acceleration);

			float GetAccellerationTime() const;
			void SetAccellerationTime(float time);

			float GetMouseSensitivity() const;
			void SetMouseSensitivity(float sensitivity);

			bool HandleEvent(const Event* _event);

			void SetFrozen(bool isFrozen);

			void Update(const SystemTime& systemTime);
		};
	}
}

#endif