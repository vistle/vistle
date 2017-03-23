// EngineBuildingBlocks/Input/KeyHandler.cpp

#include <EngineBuildingBlocks/Input/KeyHandler.h>

#include <Core/Constants.h>
#include <EngineBuildingBlocks/EventHandling.h>
#include <EngineBuildingBlocks/Input/Keys.h>
#include <EngineBuildingBlocks/SystemTime.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Input;

KeyEvent::KeyEvent(unsigned eventClassId, const EngineBuildingBlocks::SystemTime& systemTime)
	: Event(eventClassId)
	, SystemTime(&systemTime)
{
}

KeyHandler::KeyEventHandler::KeyEventHandler(KeyHandler* owner, unsigned eventClassId)
	: m_Owner(owner)
	, m_EventClassId(eventClassId)
{
}

KeyHandler::KeyEventHandler::~KeyEventHandler()
{
}

KeyHandler::StateKeyEventHandler::StateKeyEventHandler(KeyHandler* owner, unsigned eventClassId)
	: KeyEventHandler(owner, eventClassId)
	, m_LastState(KeyState::Invalid)
{
}

void KeyHandler::StateKeyEventHandler::Handle(const EngineBuildingBlocks::SystemTime& systemTime)
{
	Keys key = m_Owner->GetEventKey(m_EventClassId);
	if (key != Keys::Unknown)
	{
		KeyState keyState = m_Owner->GetKeyState(key);
		if (m_LastState == KeyState::Released && keyState == KeyState::Pressed)
		{
			m_Owner->PostKeyEvent({ m_EventClassId, systemTime });
		}
		m_LastState = keyState;
	}
}

KeyHandler::TimedKeyEventHandler::TimedKeyEventHandler(KeyHandler* owner, unsigned eventClassId)
	: KeyEventHandler(owner, eventClassId)
{
}

void KeyHandler::TimedKeyEventHandler::Handle(const EngineBuildingBlocks::SystemTime& systemTime)
{
	Keys key = m_Owner->GetEventKey(m_EventClassId);
	if (key != Keys::Unknown)
	{
		KeyState keyState = m_Owner->GetKeyState(key);
		if (keyState == KeyState::Pressed)
		{
			m_Owner->PostKeyEvent({ m_EventClassId, systemTime });
		}
	}
}

KeyHandler::KeyHandler(EngineBuildingBlocks::EventManager* eventManager)
	: m_EventManager(eventManager)
	, m_KeyStateProvider(nullptr)
{
}

KeyHandler::~KeyHandler()
{
	for (unsigned i = 0; i < m_KeyEventHandlers.GetSize(); i++)
	{
		delete m_KeyEventHandlers[i];
	}
}

void KeyHandler::SetKeyStateProvider(IKeyStateProvider* keyStateProvider)
{
	this->m_KeyStateProvider = keyStateProvider;
}

void KeyHandler::Initialize()
{
	m_KeyState.Resize(static_cast<int>(Keys::COUNT));
	if (m_KeyStateProvider != nullptr)
	{
		for (unsigned i = 0; i < m_KeyState.GetSize(); i++)
		{
			m_KeyState[i] = m_KeyStateProvider->GetKeyState((Keys)i);
		}
	}
	else // Assuming that every key is released.
	{
		for (unsigned i = 0; i < m_KeyState.GetSize(); i++)
		{
			m_KeyState[i] = KeyState::Released;
		}
	}
}

void KeyHandler::Update(const SystemTime& systemTime)
{
	m_CurrentKeyEvents.ReleaseAll();
	for (unsigned i = 0; i < m_KeyEventHandlers.GetSize(); i++)
	{
		m_KeyEventHandlers[i]->Handle(systemTime);
	}
}

KeyState KeyHandler::GetKeyState(Keys key)
{
	if (key == Keys::Unknown)
	{
		return KeyState::Unhandled;
	}

	return m_KeyState[static_cast<int>(key)];
}

void KeyHandler::OnKeyAction(Keys key, KeyState keyState)
{
	if (key != Keys::Unknown)
	{
		m_KeyState[static_cast<int>(key)] = keyState;
	}
}

void KeyHandler::BindEventToKey(unsigned eventClassId, Keys key)
{
	// Valid in uninitialized state.

	m_EventMap[eventClassId] = key;
}

Keys KeyHandler::GetEventKey(unsigned eventClassId)
{
	// Valid in uninitialized state.

	auto kIt = m_EventMap.find(eventClassId);
	if (kIt == m_EventMap.end())
	{
		return Keys::Unknown;
	}
	return kIt->second;
}

unsigned KeyHandler::RegisterTimedKeyEventListener(const std::string& eventName, IEventListener* eventListener)
{
	// Valid in uninitialized state.

	std::string keyEventName = "KeyEvent_Timed_" + eventName;

	unsigned eventClassId = m_EventManager->GetEventClassId(keyEventName.c_str());
	if (eventClassId == Core::c_InvalidIndexU)
	{
		eventClassId = m_EventManager->RegisterEventClass(0, keyEventName.c_str());
	}
	m_EventManager->RegisterEventListener(eventClassId, eventListener);

	m_KeyEventHandlers.PushBack(new TimedKeyEventHandler(this, eventClassId));

	return eventClassId;
}

Core::IndexVectorU KeyHandler::RegisterTimedKeyEventListener(const std::vector<std::string>& eventNames, IEventListener* eventListener)
{
	// Valid in uninitialized state.

	Core::IndexVectorU eventClassIds;
	for (size_t i = 0; i < eventNames.size(); i++)
	{
		eventClassIds.PushBack(RegisterTimedKeyEventListener(eventNames[i], eventListener));
	}
	return eventClassIds;
}

unsigned KeyHandler::RegisterStateKeyEventListener(const std::string& eventName, IEventListener* eventListener)
{
	// Valid in uninitialized state.

	std::string keyEventName = "KeyEvent_State_" + eventName;

	unsigned eventClassId = m_EventManager->GetEventClassId(keyEventName.c_str());
	if (eventClassId == Core::c_InvalidIndexU)
	{
		eventClassId = m_EventManager->RegisterEventClass(0, keyEventName.c_str());
	}
	m_EventManager->RegisterEventListener(eventClassId, eventListener);

	m_KeyEventHandlers.PushBack(new StateKeyEventHandler(this, eventClassId));

	return eventClassId;
}

Core::IndexVectorU KeyHandler::RegisterStateKeyEventListener(const std::vector<std::string>& eventNames, IEventListener* eventListener)
{
	// Valid in uninitialized state.

	Core::IndexVectorU eventClassIds;
	for (size_t i = 0; i < eventNames.size(); i++)
	{
		eventClassIds.PushBack(RegisterStateKeyEventListener(eventNames[i], eventListener));
	}
	return eventClassIds;
}

void KeyHandler::PostKeyEvent(const KeyEvent& keyEvent)
{
	auto pKeyEvent = m_CurrentKeyEvents.Request(keyEvent);
	m_EventManager->PostEvent(pKeyEvent);
}