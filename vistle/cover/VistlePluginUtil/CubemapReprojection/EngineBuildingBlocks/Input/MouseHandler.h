// EngineBuildingBlocks/Input/MouseHandler.h

#ifndef _ENGINEBUILDINGBLOCKS_MOUSEHANDLER_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_MOUSEHANDLER_H_INCLUDED_

#include <EngineBuildingBlocks/EventHandling.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/DataStructures/Pool.hpp>

#include <map>

namespace EngineBuildingBlocks
{
	class SystemTime;

	namespace Input
	{
		enum class MouseButton : char
		{
			Unhandled = -1,
			Left,
			Middle,
			Right,

			COUNT_HANDLED,
			COUNT
		};

		enum class MouseButtonState : unsigned char
		{
			Pressed,
			Released
		};

		class IMouseStateProvider
		{
		public:

			virtual MouseButtonState GetMouseButtonState(MouseButton button) const = 0;
			virtual void GetCursorPosition(float& cursorPositionX, float& cursorPositionY) const = 0;
		};

		struct MouseEvent : public Event
		{
			MouseEvent() {}
			MouseEvent(unsigned eventClassId);
		};

		struct MouseDragEvent : public MouseEvent
		{
		public:

			float FromX, FromY;
			float ToX, ToY;

			MouseDragEvent() {}
			MouseDragEvent(unsigned eventClassId, float fromX, float fromY, float toX, float toY);

			void GetDifference(float& differenceX, float& differenceY) const;
		};

		inline const MouseDragEvent& ToMouseDragEvent(const Event* _event)
		{
			return *static_cast<const MouseDragEvent*>(_event);
		}

		class MouseHandler
		{
			EventManager* m_EventManager;

			// Mouse state.
			IMouseStateProvider* m_MouseStateProvider;
			float m_CursorPositionX, m_CursorPositionY;
			std::map<MouseButton, MouseButtonState> m_MouseButtonStates;

			// Mouse event handling.
			class MouseEventHandler
			{
			protected:

				MouseHandler* m_Owner;
				unsigned m_EventClassId;

			public:

				MouseEventHandler(MouseHandler* owner, unsigned eventClassId);
				virtual ~MouseEventHandler();

				virtual void Handle(const SystemTime& systemTime) = 0;
			};

			class MouseDragEventHandler : public MouseEventHandler
			{
			protected:

				float m_LastCursorPositionX, m_LastCursorPositionY;
				bool m_IsInDrag;

				void Handle(const SystemTime& systemTime);

			public:

				MouseDragEventHandler(MouseHandler* owner, unsigned eventClassId);
			};

			Core::SimpleTypeVectorU<MouseEventHandler*> m_MouseEventHandlers;
			Core::ResourcePoolU<MouseDragEvent> m_CurrentMouseDragEvents;
			std::map<unsigned, MouseButton> m_EventMap;

		public:

			MouseHandler(EventManager* eventManager);
			~MouseHandler();

			void SetMouseStateProvider(IMouseStateProvider* mouseStateProvider);

			void BindEventToButton(unsigned eventClassId, MouseButton button);
			MouseButton GetEventButton(unsigned eventClassId) const;

			void GetCursorPosition(float& positionX, float& positionY);
			MouseButtonState GetMouseButtonState(MouseButton button);

			void OnCursorPositionChanged(float positionX, float positionY);
			void OnMouseButtonAction(MouseButton button, MouseButtonState state);

			unsigned RegisterMouseDragEventListener(const std::string& eventName, IEventListener* mouseEventListener);
			Core::IndexVectorU RegisterMouseDragEventListener(
				const Core::SimpleTypeVectorU<std::string>& eventNames, IEventListener* mouseEventListener);
		
			void Initialize();
			void Update(const SystemTime& systemTime);

			void PostMouseEvent(const MouseDragEvent& mouseDragEvent);
		};
	}
}

#endif