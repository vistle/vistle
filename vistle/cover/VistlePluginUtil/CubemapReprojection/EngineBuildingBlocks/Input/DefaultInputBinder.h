// EngineBuildingBlocks/Input/DefaultInputBinder.h

#ifndef _ENGINEBUILDINGBLOCKS_DEFAULTINPUTBINDER_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_DEFAULTINPUTBINDER_H_INCLUDED_

#include <Core/Constants.h>
#include <EngineBuildingBlocks/Input/KeyHandler.h>
#include <EngineBuildingBlocks/Input/MouseHandler.h>
#include <EngineBuildingBlocks/Graphics/Camera/FreeCamera.h>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		class FreeCamera;
	}

	namespace Input
	{
		class KeyHandler;
		class MouseHandler;

		class DefaultInputBinder
		{
		public:

			static void Bind(KeyHandler* pKeyHandler, MouseHandler* pMouseHandler,
				const Graphics::FreeCamera* pCamera)
			{
				if (pKeyHandler != nullptr)
				{
					if (pCamera != nullptr)
					{
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_MoveForward_ECI, Keys::W);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_MoveBackward_ECI, Keys::S);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_MoveLeft_ECI, Keys::A);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_MoveRight_ECI, Keys::D);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_MoveUp_ECI, Keys::KeyPad8);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_MoveDown_ECI, Keys::KeyPad2);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_IncreaseMoveSpeed_ECI, Keys::R);
						pKeyHandler->BindEventToKey(pCamera->KeyEventName_DecreaseMoveSpeed_ECI, Keys::E);
					}
				}
				if (pMouseHandler != nullptr)
				{
					if (pCamera != nullptr)
					{
						pMouseHandler->BindEventToButton(pCamera->MouseEventName_Rotate_ECI, MouseButton::Left);
					}
				}
			}
		};
	}
}

#endif