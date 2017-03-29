// EngineBuildingBlocks/Input/KeyHandler.h

#ifndef _ENGINEBUILDINGBLOCKS_KEYHANDLER_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_KEYHANDLER_H_INCLUDED_

#include <EngineBuildingBlocks/Input/Keys.h>

#include <Core/DataStructures/Pool.hpp>
#include <EngineBuildingBlocks/EventHandling.h>

#include <string>
#include <vector>
#include <map>

namespace EngineBuildingBlocks
{
	class EventManager;
	class SystemTime;

	namespace Input
	{
		enum class KeyState : char
		{
			Invalid = -2,
			Unhandled,
			Pressed,
			Released
		};

		class IKeyStateProvider
		{
		public:

			virtual KeyState GetKeyState(Keys key) const = 0;
		};

		struct KeyEvent : public Event
		{
			KeyEvent() : SystemTime(nullptr) {}
			KeyEvent(unsigned eventClassId, const SystemTime& systemTime);

			const EngineBuildingBlocks::SystemTime* SystemTime;
		};

		inline const KeyEvent& ToKeyEvent(const Event* _event)
		{
			return *static_cast<const KeyEvent*>(_event);
		}

		class KeyHandler
		{
			EventManager* m_EventManager;

			// Key state.
			IKeyStateProvider* m_KeyStateProvider;
			Core::SimpleTypeVectorU<KeyState> m_KeyState;

			// Key event handling.

			class KeyEventHandler
			{
			protected:

				KeyHandler* m_Owner;
				unsigned m_EventClassId;

			public:

				KeyEventHandler(KeyHandler* owner, unsigned eventId);
				virtual ~KeyEventHandler();

				virtual void Handle(const SystemTime& systemTime) = 0;
			};

			class StateKeyEventHandler : public KeyEventHandler
			{
				KeyState m_LastState;

			public:

				StateKeyEventHandler(KeyHandler* owner, unsigned eventId);
				void Handle(const SystemTime& systemTime);
			};

			class TimedKeyEventHandler : public KeyEventHandler
			{
			public:

				TimedKeyEventHandler(KeyHandler* owner, unsigned eventId);
				void Handle(const SystemTime& systemTime);
			};

			Core::SimpleTypeVectorU<KeyEventHandler*> m_KeyEventHandlers;
			Core::ResourcePoolU<KeyEvent> m_CurrentKeyEvents;
			std::map<unsigned, Keys> m_EventMap;

		public:

			KeyHandler(EventManager* eventManager);
			~KeyHandler();

			void SetKeyStateProvider(IKeyStateProvider* keyStateProvider);
			KeyState GetKeyState(Keys key);

			void OnKeyAction(Keys key, KeyState keyState);

			void BindEventToKey(unsigned eventId, Keys key);
			Keys GetEventKey(unsigned eventId);

			unsigned RegisterTimedKeyEventListener(const std::string& eventName, IEventListener* eventListener);
			Core::IndexVectorU RegisterTimedKeyEventListener(const std::vector<std::string>& eventNames, IEventListener* eventListener);

			unsigned RegisterStateKeyEventListener(const std::string& eventName, IEventListener* eventListener);
			Core::IndexVectorU RegisterStateKeyEventListener(const std::vector<std::string>& eventNames, IEventListener* eventListener);

			void Initialize();
			void Update(const SystemTime& systemTime);

			void PostKeyEvent(const KeyEvent& keyEvent);
		};
	}
}

#endif