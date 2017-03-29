// EngineBuildingBlocks/Input/MouseHandler.cpp

#include <EngineBuildingBlocks/Input/MouseHandler.h>

#include <Core/Constants.h>
#include <EngineBuildingBlocks/SystemTime.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Input;

MouseEvent::MouseEvent(unsigned eventClassId)
	: Event(eventClassId)
{
}

MouseDragEvent::MouseDragEvent(unsigned eventClassId, float fromX, float fromY, float toX, float toY)
	: MouseEvent(eventClassId)
	, FromX(fromX)
	, FromY(fromY)
	, ToX(toX)
	, ToY(toY)
{
}

void MouseDragEvent::GetDifference(float& differenceX, float& differenceY) const
{
	differenceX = ToX - FromX;
	differenceY = ToY - FromY;
}

MouseHandler::MouseEventHandler::MouseEventHandler(MouseHandler* owner, unsigned eventClassId)
	: m_Owner(owner)
	, m_EventClassId(eventClassId)
{
}

MouseHandler::MouseEventHandler::~MouseEventHandler()
{
}

MouseHandler::MouseDragEventHandler::MouseDragEventHandler(MouseHandler* owner, unsigned eventClassId)
	: MouseEventHandler(owner, eventClassId)
	, m_LastCursorPositionX(0.0f)
	, m_LastCursorPositionY(0.0f)
	, m_IsInDrag(false)
{
}

void MouseHandler::MouseDragEventHandler::Handle(const SystemTime& systemTime)
{
	MouseButton button = m_Owner->GetEventButton(m_EventClassId);
	if (button == MouseButton::Unhandled)
	{
		return;
	}
	
	MouseButtonState buttonState = m_Owner->GetMouseButtonState(button);
	if (buttonState == MouseButtonState::Pressed)
	{
		float currentCursorPositionX, currentCursorPositionY;
		m_Owner->GetCursorPosition(currentCursorPositionX, currentCursorPositionY);
		if (m_IsInDrag)
		{
			if (currentCursorPositionX != m_LastCursorPositionX
				|| currentCursorPositionY != m_LastCursorPositionY)
			{
				m_Owner->PostMouseEvent(MouseDragEvent(m_EventClassId,
					m_LastCursorPositionX, m_LastCursorPositionY,
					currentCursorPositionX, currentCursorPositionY));
			}
		}
		else
		{
			m_IsInDrag = true;
		}
		m_LastCursorPositionX = currentCursorPositionX;
		m_LastCursorPositionY = currentCursorPositionY;
	}
	else
	{
		m_IsInDrag = false;
	}
}

MouseHandler::MouseHandler(EventManager* eventManager)
	: m_EventManager(eventManager)
	, m_MouseStateProvider(nullptr)
{
}

MouseHandler::~MouseHandler()
{
	for (unsigned i = 0; i < m_MouseEventHandlers.GetSize(); i++)
	{
		delete m_MouseEventHandlers[i];
	}
}

void MouseHandler::SetMouseStateProvider(IMouseStateProvider* mouseStateProvider)
{
	this->m_MouseStateProvider = mouseStateProvider;
}

void MouseHandler::Initialize()
{
	if (m_MouseStateProvider != nullptr)
	{
		m_MouseStateProvider->GetCursorPosition(m_CursorPositionX, m_CursorPositionY);
		m_MouseButtonStates[MouseButton::Left] = m_MouseStateProvider->GetMouseButtonState(MouseButton::Left);
		m_MouseButtonStates[MouseButton::Middle] = m_MouseStateProvider->GetMouseButtonState(MouseButton::Middle);
		m_MouseButtonStates[MouseButton::Right] = m_MouseStateProvider->GetMouseButtonState(MouseButton::Right);
	}
}

void MouseHandler::BindEventToButton(unsigned eventClassId, MouseButton button)
{
	// Valid in uninitialized state.

	m_EventMap[eventClassId] = button;
}

MouseButton MouseHandler::GetEventButton(unsigned eventClassId) const
{
	// Valid in uninitialized state.

	auto kIt = m_EventMap.find(eventClassId);
	if (kIt == m_EventMap.end())
	{
		return MouseButton::Unhandled;
	}
	return kIt->second;
}

void MouseHandler::GetCursorPosition(float& positionX, float& positionY)
{
	positionX = this->m_CursorPositionX;
	positionY = this->m_CursorPositionY;
}

void MouseHandler::OnCursorPositionChanged(float positionX, float positionY)
{
	this->m_CursorPositionX = positionX;
	this->m_CursorPositionY = positionY;
}

MouseButtonState MouseHandler::GetMouseButtonState(MouseButton button)
{
	return m_MouseButtonStates[button];
}

void MouseHandler::OnMouseButtonAction(MouseButton button, MouseButtonState state)
{
	m_MouseButtonStates[button] = state;
}

void MouseHandler::Update(const SystemTime& systemTime)
{
	m_CurrentMouseDragEvents.ReleaseAll();
	for (unsigned i = 0; i < m_MouseEventHandlers.GetSize(); i++)
	{
		m_MouseEventHandlers[i]->Handle(systemTime);
	}
}

unsigned MouseHandler::RegisterMouseDragEventListener(const std::string& eventName, IEventListener* mouseEventListener)
{
	// Valid in uninitialized state.

	std::string mouseEventName = "MouseEvent_Drag_" + eventName;

	unsigned eventClassId = m_EventManager->GetEventClassId(mouseEventName.c_str());
	if (eventClassId == Core::c_InvalidIndexU)
	{
		eventClassId = m_EventManager->RegisterEventClass(0, mouseEventName.c_str());
	}
	m_EventManager->RegisterEventListener(eventClassId, mouseEventListener);

	m_MouseEventHandlers.PushBack(new MouseDragEventHandler(this, eventClassId));

	return eventClassId;
}

Core::IndexVectorU MouseHandler::RegisterMouseDragEventListener(
	const Core::SimpleTypeVectorU<std::string>& eventNames, IEventListener* mouseEventListener)
{
	// Valid in uninitialized state.

	Core::IndexVectorU eventClassIds;
	for (unsigned i = 0; i < eventNames.GetSize(); i++)
	{
		eventClassIds.PushBack(RegisterMouseDragEventListener(eventNames[i], mouseEventListener));
	}
	return eventClassIds;
}

void MouseHandler::PostMouseEvent(const MouseDragEvent& mouseDragEvent)
{
	auto pMouseDragEvent = m_CurrentMouseDragEvents.Request(mouseDragEvent);
	m_EventManager->PostEvent(pMouseDragEvent);
}