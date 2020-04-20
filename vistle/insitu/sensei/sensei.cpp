#include "sensei.h"
#include <insitu/core/dataAdaptor.h>

bool insitu::sensei::SenseiAdapter::Initialize()
{
	try
	{
		//startListening();
		//dumpConnectionFile();
	}
	catch (const std::exception&)
	{
		return false;
	}
	return true;
}

insitu::sensei::SenseiAdapter::~SenseiAdapter()
{
	delete m_sendMessageQueue;
}
