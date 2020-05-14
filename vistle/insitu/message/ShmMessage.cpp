#include "ShmMessage.h"

void insitu::message::InSituShmMessage::initialize()
{
	bool error = false;
	int iter = 0;
	do
	{
		error = false;
		try
		{
			m_msqName += std::to_string(iter++);
			m_msqs[0] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::create_only, (m_msqName + "_recv" ).c_str(), ShmMessageQueueLenght, sizeof(ShmMsg));
			m_msqs[1] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::create_only, (m_msqName +  "_send").c_str(), ShmMessageQueueLenght, sizeof(ShmMsg));
		}
		catch (const boost::interprocess::interprocess_exception& ex)
		{
			std::cerr << "ShmMessage " << m_msqName << " creation error: " << ex.what() << std::endl;
			error = true;
		}
	} while (error);
	m_order = { 0,1 };
}

void insitu::message::InSituShmMessage::initialize(const std::string& msqName)
{
	try
	{
		m_msqs[0] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only, (m_msqName + "_send").c_str());
		m_msqs[1] = std::make_unique<boost::interprocess::message_queue>(boost::interprocess::open_only, (m_msqName + "_recv").c_str());
	}
	catch (const boost::interprocess::interprocess_exception& ex)
	{
		std::cerr << "ShmMessage " << m_msqName << " opening error: " << ex.what() << std::endl;
	}
	m_order = { 1,0 };
}

bool insitu::message::InSituShmMessage::isInitialized()
{
	return m_initialized;
}

std::string insitu::message::InSituShmMessage::name()
{
	return m_msqName;
}

insitu::message::Message insitu::message::InSituShmMessage::recv()
{
	vistle::buffer payload;
	int type = 0;
	bool first = true;
	auto iter = payload.begin();
	unsigned int i = 0;

	ShmMsg m;
	size_t recvSize = 0;
	unsigned int prio = 0;

	while (iter != payload.end())
	{
		try
		{
			m_msqs[m_order[0]]->receive((void*)&m, sizeof(m), recvSize, prio);
		}
		catch (const boost::interprocess::interprocess_exception& ex)
		{
			std::cerr << "ShmMessage receive error: " << ex.what() << std::endl;
			return Message::errorMessage();
		}
		if (first)
		{
			payload.resize(m.size);
			type = m.type;
			iter = payload.begin();
		}
		assert(type == m.type);
		int left = ShmMessageMaxSize;
		if (m.size < i++ * ShmMessageMaxSize)
		{
			left = m.size - (i - 1) * ShmMessageMaxSize;
		}
		std::copy(m.buf, m.buf + left, iter);
		iter += left;
	}
	return Message{ static_cast<InSituMessageType>(type), std::move(payload) };
}

insitu::message::Message insitu::message::InSituShmMessage::tryRecv()
{
	vistle::buffer payload;
	int type = 0;
	auto iter = payload.begin();
	unsigned int i = 0;

	ShmMsg m;
	size_t recvSize = 0;
	unsigned int prio = 0;

	try
	{
		if (!m_msqs[m_order[0]]->try_receive((void*)&m, sizeof(m), recvSize, prio))
		{
			return Message::errorMessage();
		}
		payload.resize(m.size);
		type = m.type;
		iter = payload.begin();
		int left = ShmMessageMaxSize;
		if (m.size < i++ * ShmMessageMaxSize)
		{
			left = m.size - (i - 1) * ShmMessageMaxSize;
		}
		std::copy(m.buf, m.buf + left, iter);
	}
	catch (const boost::interprocess::interprocess_exception& ex)
	{
		std::cerr << "ShmMessage tryReceive error: " << ex.what() << std::endl;
		return Message::errorMessage();
	}

	while (i * ShmMessageMaxSize < payload.size())
	{
		try
		{
			m_msqs[m_order[0]]->receive((void*)&m, sizeof(m), recvSize, prio);
		}
		catch (const boost::interprocess::interprocess_exception& ex)
		{
			std::cerr << "ShmMessage tryReceive error: " << ex.what() << std::endl;
			return Message::errorMessage();
		}

		assert(type == m.type);
		int left = ShmMessageMaxSize;
		if (m.size < i++ * ShmMessageMaxSize)
		{
			left = m.size - (i - 1) * ShmMessageMaxSize;
		}
		std::copy(m.buf, m.buf + left, iter);
	}
	return Message{ static_cast<InSituMessageType>(type), std::move(payload) };
}

insitu::message::Message insitu::message::InSituShmMessage::timedRecv(size_t timeInSec)
{
	vistle::buffer payload;
	int type = 0;
	auto iter = payload.begin();
	unsigned int i = 0;

	ShmMsg m;
	size_t recvSize = 0;
	unsigned int prio = 0;
	boost::posix_time::ptime endTime(boost::posix_time::second_clock::local_time());
	endTime += boost::posix_time::seconds(timeInSec);
	try
	{
		if (!m_msqs[m_order[0]]->timed_receive((void*)&m, sizeof(m), recvSize, prio, endTime))
		{
			return Message::errorMessage();
		}
		payload.resize(m.size);
		type = m.type;
		iter = payload.begin();
		int left = ShmMessageMaxSize;
		if (m.size < i++ * ShmMessageMaxSize)
		{
			left = m.size - (i - 1) * ShmMessageMaxSize;
		}
		std::copy(m.buf, m.buf + left, iter);
	}
	catch (const boost::interprocess::interprocess_exception& ex)
	{
		std::cerr << "ShmMessage timedRecv error: " << ex.what() << std::endl;
		return Message::errorMessage();
	}

	while (i * ShmMessageMaxSize < payload.size())
	{
		try
		{
			if (!m_msqs[m_order[0]]->timed_receive((void*)&m, sizeof(m), recvSize, prio, endTime))
			{
				return Message::errorMessage();
			}
		}
		catch (const boost::interprocess::interprocess_exception& ex)
		{
			std::cerr << "ShmMessage timedRecv error: " << ex.what() << std::endl;
			return Message::errorMessage();
		}

		assert(type == m.type);
		int left = ShmMessageMaxSize;
		if (m.size < i++ * ShmMessageMaxSize)
		{
			left = m.size - (i - 1) * ShmMessageMaxSize;
		}
		std::copy(m.buf, m.buf + left, iter);
	}
	return Message{ static_cast<InSituMessageType>(type), std::move(payload) };
}
