// EngineBuildingBlocks/EventHandling.cpp

#include <EngineBuildingBlocks/EventHandling.h>

#include <Core/Constants.h>
#include <Core/Platform.h>

#include <cassert>

using namespace EngineBuildingBlocks;

Event::Event(unsigned classId)
	: ClassId(classId)
{
}

EventManager::EventManager()
	: m_IsHandlingEvents(false)
	, m_CountEvents(0)
	, m_StartPriority(Core::c_InvalidIndexU)
	, m_DefaultNameIndex(0)
{
}

unsigned EventManager::RegisterEventClass(unsigned priority, const char* name)
{
	return RegisterEventClass(priority, priority, priority, name);
}

unsigned EventManager::RegisterEventClass(unsigned startPriority, unsigned endPriority, unsigned defaultPriority,
	const char* eventClassName)
{
	assert(defaultPriority >= startPriority && defaultPriority <= endPriority);
	assert(!m_IsHandlingEvents);

	assert(!HasEventClass(eventClassName));

	unsigned eventClassId = static_cast<unsigned>(m_EventClassData.size());

	char defaultName[32];
	if (eventClassName == nullptr)
	{
#ifdef IS_WINDOWS
		sprintf_s(defaultName, 32, "_UnnamedEvent[%d]", m_DefaultNameIndex++);
#else
		sprintf(defaultName, "_UnnamedEvent[%d]", m_DefaultNameIndex++);
#endif
		eventClassName = defaultName;
	}

	m_EventClassData.push_back({ eventClassId, startPriority, endPriority, defaultPriority, eventClassName });

	while (m_Events.size() <= endPriority)
	{
		m_Events.emplace_back();
	}

	return eventClassId;
}

bool EventManager::HasEventClass(const char* eventClassName) const
{
	return (std::find_if(m_EventClassData.begin(), m_EventClassData.end(),
		[=](const EventClassData & element) { return element.Name == eventClassName; })
			!= m_EventClassData.end());
}

unsigned EventManager::GetEventClassId(const char* eventClassName) const
{
	auto it = std::find_if(m_EventClassData.begin(), m_EventClassData.end(),
		[=](const EventClassData & element) { return element.Name == eventClassName; });
	if (it == m_EventClassData.end())
	{
		return Core::c_InvalidIndexU;
	}
	return it->ClassId;
}

void EventManager::RegisterEventListenerThreadSafe(unsigned eventClassId, IEventListener* eventListener)
{
	m_Mutex.lock();
	RegisterEventListener(eventClassId, eventListener);
	m_Mutex.unlock();
}

void EventManager::RegisterEventListener(unsigned eventClassId, IEventListener* eventListener)
{
	m_EventClassData[eventClassId].Listeners.push_back(eventListener);
}

const EventClassData& EventManager::GetEventClassData(unsigned eventClassId) const
{
	return m_EventClassData[eventClassId];
}

void EventManager::PostEventThreadSafe(const Event* _event)
{
	m_Mutex.lock();
	PostEvent(_event);
	m_Mutex.unlock();
}

void EventManager::PostEventThreadSafe(const Event* _event, unsigned priority)
{
	m_Mutex.lock();
	PostEvent(_event, priority);
	m_Mutex.unlock();
}

void EventManager::PostEvent(const Event* _event)
{
	PostEvent(_event, m_EventClassData[_event->ClassId].DefaultPriority);
}

void EventManager::PostEvent(const Event* _event, unsigned priority)
{
	assert(priority >= m_EventClassData[_event->ClassId].StartPriority && priority <= m_EventClassData[_event->ClassId].EndPriority);

	m_Events[priority].push(_event);
	++m_CountEvents;
	if (priority < m_StartPriority)
	{
		m_StartPriority = priority;
	}
}

void EventManager::HandleEvents()
{
	unsigned level = m_StartPriority;
	m_StartPriority = Core::c_InvalidIndexU;

	m_IsHandlingEvents = true;

	while (m_CountEvents > 0)
	{
		bool isInterrupted = false;
		while (m_Events[level].size() > 0)
		{
			auto _event = m_Events[level].front();
			m_Events[level].pop();
			unsigned countEventsBeforeFiring = --m_CountEvents;

			FireEvent(_event);

			if (m_CountEvents > countEventsBeforeFiring)
			{
				unsigned startPriority = m_StartPriority;
				m_StartPriority = Core::c_InvalidIndexU;
				if (level > startPriority)
				{
					level = startPriority;
					isInterrupted = true;
					break;
				}
			}
		}
		if (!isInterrupted)
		{
			++level;
		}
	}

	m_IsHandlingEvents = false;
}

void EventManager::FireEvent(const Event* _event)
{
	auto& handlers = m_EventClassData[_event->ClassId].Listeners;
	for (size_t i = 0; i < handlers.size(); i++)
	{
		if (handlers[i]->HandleEvent(_event)) // Stopping event handling when a listener reports the event to be handled.
		{
			return;
		}
	}
}