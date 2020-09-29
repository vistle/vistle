#include "SyncShmIDs.h"
#include <vistle/core/messagequeue.h>

using namespace vistle::insitu;
using namespace vistle::insitu::message;

void SyncShmIDs::initialize(int moduleID, int rank, const std::string &suffix, Mode mode)
{
    m_moduleID = moduleID;
    m_rank = rank;
    std::string segName = vistle::message::MessageQueue::createName(("syncShmIds" + suffix).c_str(), moduleID, rank);
#ifndef MODULE_THREAD
    m_segment = ShmSegment{segName, mode};
#endif // MODULE_THREAD

    m_initialized = true;
}

void SyncShmIDs::close()
{
#ifndef MODULE_THREAD
    m_segment = ShmSegment{};
#endif // MODULE_THREAD

    m_initialized = false;
}

bool SyncShmIDs::isInitialized()
{
    return m_initialized;
}

void SyncShmIDs::set(int objID, int arrayID)
{
#ifdef MODULE_THREAD
    // do nothing
#else
    if (!m_initialized) {
        return;
    }
    try {
        Guard g(m_segment.data()->mutex);
        m_segment.data()->objID = objID;
        m_segment.data()->arrayID = arrayID;
    } catch (const boost::interprocess::interprocess_exception &ex) {
        throw vistle::exception(std::string("error setting shm object IDs: ") + ex.what());
    }
#endif // MODULE_THREAD
}

int SyncShmIDs::objectID()
{
#ifndef MODULE_THREAD
    if (m_initialized) {
        try {
            Guard g(m_segment.data()->mutex);
            return m_segment.data()->objID;
        } catch (const boost::interprocess::interprocess_exception &ex) {
            throw vistle::exception(std::string("error getting shm object ID: ") + ex.what());
        }
    }
#endif
    return vistle::Shm::the().objectID();
}

int SyncShmIDs::arrayID()
{
#ifndef MODULE_THREAD
    if (m_initialized) {
    }
    try {
        Guard g(m_segment.data()->mutex);
        return m_segment.data()->arrayID;

    } catch (const boost::interprocess::interprocess_exception &ex) {
        throw vistle::exception(std::string("error getting shm array ID: ") + ex.what());
    }
#endif
    return vistle::Shm::the().arrayID();
}

#ifndef MODULE_THREAD

SyncShmIDs::ShmSegment::ShmSegment(SyncShmIDs::ShmSegment &&other)
: m_name(std::move(other.m_name)), m_region(std::move(other.m_region))

{}

SyncShmIDs::ShmSegment::ShmSegment(const std::string &name, Mode mode): m_name(name)
{
    using namespace boost::interprocess;
    std::cerr << "trying to create shm segment with name " << name << std::endl;
    switch (mode) {
    case SyncShmIDs::Mode::Create: {
        try {
            shared_memory_object::remove(name.c_str());

            auto m_shmObj = shared_memory_object{create_only, name.c_str(), read_write};
            m_shmObj.truncate(sizeof(ShmData));
            m_region = mapped_region{m_shmObj, read_write};
            void *addr = m_region.get_address();
            new (addr) ShmData{vistle::Shm::the().objectID(),
                               vistle::Shm::the().arrayID()}; // create the ShmData object in the shared_memory_object
        } catch (boost::interprocess::interprocess_exception &ex) {
            throw vistle::exception(std::string("opening shm segment ") + name + ": " + ex.what());
        }
    }

    break;
    case SyncShmIDs::Mode::Attach: {
        try {
            auto m_shmObj = shared_memory_object{open_only, name.c_str(), read_write};
            m_region = mapped_region{m_shmObj, read_write};
            shared_memory_object::remove(name.c_str());
        } catch (boost::interprocess::interprocess_exception &ex) {
            throw vistle::exception(std::string("opening shm segment ") + name + ": " + ex.what());
        }
    } break;
    default:
        break;
    }
}

SyncShmIDs::ShmSegment::~ShmSegment()
{
    boost::interprocess::shared_memory_object::remove(m_name.c_str());
}

const SyncShmIDs::ShmData *SyncShmIDs::ShmSegment::data() const
{
    return static_cast<ShmData *>(m_region.get_address());
}
SyncShmIDs::ShmData *SyncShmIDs::ShmSegment::data()
{
    return static_cast<ShmData *>(m_region.get_address());
}

SyncShmIDs::ShmSegment &SyncShmIDs::ShmSegment::operator=(insitu::message::SyncShmIDs::ShmSegment &&other) noexcept
{
    m_name = std::move(other.m_name);
    m_region = std::move(other.m_region);
    return *this;
}

#endif // !MODULE_THREAD
