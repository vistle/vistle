#ifndef VISTLE_AVAILABLEMODULE_H
#define VISTLE_AVAILABLEMODULE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "export.h"
#include "message.h"
#include "messagepayload.h"
#include "messages.h"
#include "object.h"
#include "port.h"
namespace vistle {

const int ModuleNameLength = 50;
typedef std::function<bool(const message::Message &msg, const buffer *payload)> sendMessageFunction;
typedef std::function<bool(const message::Message &msg, const MessagePayload &payload)> sendShmMessageFunction;

class V_COREEXPORT AvailableModuleBase {
public:
    struct Key {
        int hub;
        std::string name;
        Key(int hub, const std::string &name);
        bool operator<(const Key &rhs) const;
    };
    AvailableModuleBase() = default;
    AvailableModuleBase(int hub, const std::string &name, const std::string &path, const std::string &description);
    AvailableModuleBase(AvailableModuleBase &&) = default;
    AvailableModuleBase(const AvailableModuleBase &) = delete;
    AvailableModuleBase &operator=(AvailableModuleBase &&) = default;
    AvailableModuleBase &operator=(const AvailableModuleBase &) = delete;
    virtual ~AvailableModuleBase() = default;
    int hub() const;
    const std::string &name() const;
    const std::string &path() const;
    const std::string &description() const;
    void setHub(int hubId);
    std::string print() const;
    bool isCompound() const;
    struct SubModule {
        std::string name;
        float x, y; //offset in mapeditor if split
        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &name;
            ar &x;
            ar &y;
        }
    };
    struct Connection {
        int fromId, toId; //relative to first submodule
        std::string fromPort, toPort;
        vistle::Port::Type type = vistle::Port::Type::ANY; //ANY for connectins between submodules
        bool operator<(const Connection &other) const;

        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &fromId;
            ar &toId;
            ar &fromPort;
            ar &toPort;
            ar &type;
        }
    };

    size_t addSubmodule(const SubModule &sub);
    void addConnection(const Connection &conn);
    const std::vector<SubModule> submodules() const;
    const std::set<Connection> connections() const;

protected:
    template<typename T>
    bool send(const sendMessageFunction &func) const
    {
        auto msg = T{*this};
        auto pl = addPayload(msg, *this);
        return func(msg, &pl);
    }

    template<typename T>
    bool send(const sendShmMessageFunction &func) const
    {
        auto msg = T{*this};
        auto pl = MessagePayload{addPayload(msg, *this)};
        return func(msg, pl);
    }
    template<typename T>
    void initialize(const T &msg)
    {
        m_hub = msg.hub();
        m_name = msg.name();
        m_path = msg.path();
    }

private:
    int m_hub;
    std::string m_name;

protected:
    std::string m_path;

private:
    std::string m_description;


    //for module compounds
    std::set<Connection> m_connections; //maps input-, outputports and params to parent ones
    std::vector<SubModule> m_submodules;

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &m_description;
        ar &m_connections;
        ar &m_submodules;
    }
};
const std::string moduleCompoundSuffix = ".vsl";
class ModuleCompound;
class V_COREEXPORT AvailableModule: public AvailableModuleBase {
public:
    friend class ModuleCompound;
    using AvailableModuleBase::AvailableModuleBase;
    AvailableModule(const message::ModuleAvailable &msg, const buffer &payload);

    bool send(const sendMessageFunction &func) const;
    bool send(const sendShmMessageFunction &func) const;
    void connect(int fromId, const std::string &fromName, int toId, const std::string &toName);

private:
    AvailableModule(ModuleCompound &&other);
};

class V_COREEXPORT ModuleCompound: public AvailableModuleBase {
public:
    using AvailableModuleBase::AvailableModuleBase;
    ModuleCompound(const message::CreateModuleCompound &msg, const buffer &payload);

    bool send(const sendMessageFunction &func) const;
    bool send(const sendShmMessageFunction &func) const;
    void setPath(const std::string &path);
    AvailableModule transform(); //this invalidates this object;
};
typedef std::map<AvailableModule::Key, AvailableModule> AvailableMap;


} // namespace vistle
#endif
