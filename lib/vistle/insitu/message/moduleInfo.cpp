#include "moduleInfo.h"
#include "InSituMessage.h"

using namespace vistle::insitu::message;

bool ModuleInfo::isInitialized() const
{
    return m_initialized;
}
bool ModuleInfo::isReady() const
{
    return m_ready;
}
bool ModuleInfo::isPortConnected(const std::string &portName) const
{
    return m_connectedPorts.find(portName) != m_connectedPorts.end();
}
size_t ModuleInfo::id() const
{
    return m_shmInfo.id;
}
size_t ModuleInfo::mpiSize() const
{
    return m_shmInfo.mpiSize;
}
std::string ModuleInfo::name() const
{
    return m_shmInfo.name;
}
std::string ModuleInfo::hostname() const
{
    return m_shmInfo.hostname;
}
std::string ModuleInfo::shmName() const
{
    return m_shmInfo.shmName;
}
std::string ModuleInfo::uniqueSuffix() const
{
    return std::to_string(m_shmInfo.numCons);
}

bool ModuleInfo::update(const Message &msg)
{
    switch (msg.type()) {
    case InSituMessageType::SetPorts: {
        auto m = msg.unpackOrCast<message::SetPorts>();
        for (auto portList: m.value) {
            for (auto port: portList) {
                m_connectedPorts.insert(port);
            }
        }
    } break;
    case InSituMessageType::ConnectPort: {
        auto m = msg.unpackOrCast<message::ConnectPort>();
        m_connectedPorts.insert(m.value);
    } break;
    case InSituMessageType::DisconnectPort: {
        auto m = msg.unpackOrCast<message::DisconnectPort>();
        m_connectedPorts.erase(m.value);
    } break;
    case InSituMessageType::Ready: {
        m_ready = msg.unpackOrCast<Ready>().value;
    } break;
    case InSituMessageType::ShmInfo: {
        m_shmInfo = msg.unpackOrCast<message::ShmInfo>().value;
    } break;
#ifdef MODULE_THREAD
    case InSituMessageType::ModuleID: {
        auto mid = msg.unpackOrCast<ModuleID>();
        m_shmInfo.id = mid.value;
    } break;
#endif
    default:
        return false;
    }
    return true;
}

void ModuleInfo::update(const ModuleInfo::ShmInfo &shmInfo)
{
    m_shmInfo = shmInfo;
}
void ModuleInfo::setInitState(bool state)
{
    m_initialized = state;
}
void ModuleInfo::setReadyState(bool state)
{
    m_ready = state;
}
