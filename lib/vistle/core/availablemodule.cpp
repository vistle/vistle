#include "availablemodule.h"
#include "messages.h"
#include "messagepayloadtemplates.h"
#include <cassert>
namespace vistle {

AvailableModuleBase::AvailableModuleBase(int hub, const std::string &name, const std::string &path,
                                         const std::string &description)
: m_hub(hub), m_name(name), m_path(path), m_description(description)
{}

AvailableModuleBase::AvailableModuleBase(const message::Message &msg, const buffer &payload)
{
    using namespace message;
    const ModuleBaseMessage *moduleBaseMessage;
    switch (msg.type()) {
    case MODULEAVAILABLE:
        moduleBaseMessage = static_cast<const ModuleAvailable *>(&msg);
        break;
    case CREATEMODULECOMPOUND:
        moduleBaseMessage = static_cast<const CreateModuleCompound *>(&msg);
        break;
    default:
        return;
    }
    m_hub = moduleBaseMessage->hub();
    m_name = moduleBaseMessage->name();
    m_path = moduleBaseMessage->path();

    assert(!payload.empty());
    getFromPayload(payload, *this);
}

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

const std::vector<AvailableModuleBase::SubModule> AvailableModuleBase::submodules() const
{
    return m_submodules;
}

const std::set<AvailableModuleBase::Connection> AvailableModuleBase::connections() const
{
    return m_connections;
}

bool AvailableModuleBase::send(message::Type type, const sendMessageFunction &func) const
{
    using namespace message;

    auto msg = cacheMsg(type);
    auto pl = addPayload(*msg, *this);
    return func(*msg, &pl);
}

bool AvailableModuleBase::send(message::Type type, const sendShmMessageFunction &func) const
{
    auto msg = cacheMsg(type);
    auto pl = MessagePayload{addPayload(*msg, *this)};
    assert(pl->size() > 0);
    msg->setPayloadSize(pl->size());
    return func(*msg, pl);
}

std::unique_ptr<message::Buffer> AvailableModuleBase::cacheMsg(message::Type type) const
{
    using namespace message;
    auto buf = std::make_unique<message::Buffer>();
    switch (type) {
    case MODULEAVAILABLE:
        *buf = ModuleAvailable(*this);
        break;
    case CREATEMODULECOMPOUND:
        *buf = CreateModuleCompound(*this);
        break;
    default:
        break;
    }
    return buf;
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


bool AvailableModule::send(const sendMessageFunction &func) const
{
    return AvailableModuleBase::send(message::Type::MODULEAVAILABLE, func);
}

bool AvailableModule::send(const sendShmMessageFunction &func) const
{
    return AvailableModuleBase::send(message::Type::MODULEAVAILABLE, func);
}

bool ModuleCompound::send(const sendMessageFunction &func) const
{
    return AvailableModuleBase::send(message::Type::CREATEMODULECOMPOUND, func);
}

bool ModuleCompound::send(const sendShmMessageFunction &func) const
{
    return AvailableModuleBase::send(message::Type::CREATEMODULECOMPOUND, func);
}

AvailableModule ModuleCompound::transform()
{
    return AvailableModule{std::move(*this)};
}


} // namespace vistle
