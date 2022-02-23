#include "availablemodule.h"
#include "messagepayloadtemplates.h"
#include "messages.h"
#include <cassert>
namespace vistle {


AvailableModuleBase::AvailableModuleBase(int hub, const std::string &name, const std::string &path,
                                         const std::string &description)
: m_hub(hub), m_name(name), m_path(path), m_description(description)
{}

int AvailableModuleBase::hub() const
{
    return m_hub;
}


const std::string &AvailableModuleBase::name() const
{
    return m_name;
}


const std::string &AvailableModuleBase::path() const
{
    return m_path;
}


const std::string &AvailableModuleBase::description() const
{
    return m_description;
}


void AvailableModuleBase::setHub(int hubId)
{
    m_hub = hubId;
}


size_t AvailableModuleBase::addSubmodule(const AvailableModuleBase::SubModule &sub)
{
    m_submodules.push_back(sub);
    return m_submodules.size() - 1;
}


void AvailableModuleBase::addConnection(const AvailableModuleBase::Connection &conn)
{
    m_connections.insert(conn);
}


const std::vector<typename AvailableModuleBase::SubModule> AvailableModuleBase::submodules() const
{
    return m_submodules;
}


const std::set<typename AvailableModuleBase::Connection> AvailableModuleBase::connections() const
{
    return m_connections;
}

std::string AvailableModuleBase::print() const
{
    std::stringstream ss;
    ss << "AvailableModule: " << name() << " from hub" << hub() << std::endl;
    if (!m_submodules.empty()) {
        ss << "This is a module compound consisting of " << m_submodules.size() << " submodules:" << std::endl;
        for (const auto &sub: m_submodules)
            ss << sub.name << std::endl;
        for (const auto &conn: m_connections) {
            ss << "conn ids: " << conn.fromId << " " << conn.toId << std::endl;
            auto from = conn.fromId == -1 ? "compound" : m_submodules[conn.fromId].name;
            auto to = conn.toId == -1 ? "compound" : m_submodules[conn.toId].name;

            ss << from << "'s port " << conn.fromPort << " is connected to " << to << "'s port " << conn.toPort
               << std::endl;
        }
    }
    return ss.str();
}


bool AvailableModuleBase::isCompound() const
{
    return m_submodules.size();
}


AvailableModuleBase::Key::Key(int hub, const std::string &name): hub(hub), name(name)
{}


bool AvailableModuleBase::Key::operator<(const Key &rhs) const
{
    if (hub == rhs.hub) {
        return name < rhs.name;
    }
    return hub < rhs.hub;
}

AvailableModule::AvailableModule(ModuleCompound &&other): AvailableModuleBase(std::move(other))
{}

AvailableModule::AvailableModule(const message::ModuleAvailable &msg, const buffer &payload)
{
    initialize(msg);

    assert(!payload.empty());
    message::getFromPayload(payload, *this);
}

bool AvailableModule::send(const sendMessageFunction &func) const
{
    return AvailableModuleBase::send<message::ModuleAvailable>(func);
}

bool AvailableModule::send(const sendShmMessageFunction &func) const
{
    return AvailableModuleBase::send<message::ModuleAvailable>(func);
}

ModuleCompound::ModuleCompound(const message::CreateModuleCompound &msg, const buffer &payload)
{
    initialize(msg);
    assert(!payload.empty());
    message::getFromPayload(payload, *this);
}

bool ModuleCompound::send(const sendMessageFunction &func) const
{
    return AvailableModuleBase::send<message::CreateModuleCompound>(func);
}

bool ModuleCompound::send(const sendShmMessageFunction &func) const
{
    return AvailableModuleBase::send<message::CreateModuleCompound>(func);
}

AvailableModule ModuleCompound::transform()
{
    return AvailableModule{std::move(*this)};
}


} // namespace vistle
