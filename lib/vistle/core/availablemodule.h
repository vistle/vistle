#ifndef VISTLE_AVAILABLEMODULE_H
#define VISTLE_AVAILABLEMODULE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "export.h"
#include "message.h"
#include "messages.h"
#include "messagepayload.h"
#include "object.h"
namespace vistle {

const int ModuleNameLength = 50;
typedef std::function<bool(const message::Message &msg, const buffer *payload)> sendMessageFunction;
typedef std::function<bool(const message::Message &msg, const MessagePayload &payload)> sendShmMessageFunction;

class V_COREEXPORT AvailableModuleBase {
public:
    friend class ModuleBaseMessage;
    struct Key {
        int hub;
        std::string name;
        Key(int hub, const std::string &name);
        bool operator<(const Key &rhs) const;
    };
    AvailableModuleBase() = default;
    AvailableModuleBase(int hub, const std::string &name, const std::string &path, const std::string &description);
    AvailableModuleBase(const message::Message &msg, const buffer &payload);
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
        bool operator<(const Connection &other) const
        {
            bool retval = other.fromId < fromId;
            if (other.fromId == fromId)
                retval = other.toId < toId;
            else
                return retval;
            if (other.toId == toId)
                retval = other.fromPort < fromPort;
            else
                return retval;
            if (other.fromPort == fromPort)
                retval = other.toPort < toPort;
            return retval;
        }
        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &fromId;
            ar &toId;
            ar &fromPort;
            ar &toPort;
        }
    };

    size_t addSubmodule(const SubModule &sub);
    void addConnection(const Connection &conn);
    const std::vector<SubModule> submodules() const;
    const std::set<Connection> connections() const;

protected:
    bool send(message::Type type, const sendMessageFunction &func) const;
    bool send(message::Type type, const sendShmMessageFunction &func) const;

private:
    int m_hub;
    std::string m_name;
    std::string m_path;
    std::string m_description;


    //for module compounds
    std::set<Connection> m_connections; //maps input-, outputports and params to parent ones
    std::vector<SubModule> m_submodules;

    mutable vistle::buffer m_cacheBuffer;
    mutable vistle::MessagePayload m_cacheMessagePayload;
    mutable bool m_changed = true;
    std::unique_ptr<message::Buffer> cacheMsg(message::Type type) const;

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
    bool send(const sendMessageFunction &func) const;
    bool send(const sendShmMessageFunction &func) const;

private:
    AvailableModule(ModuleCompound &&other);
};

class V_COREEXPORT ModuleCompound: public AvailableModuleBase {
public:
    using AvailableModuleBase::AvailableModuleBase;
    bool send(const sendMessageFunction &func) const;
    bool send(const sendShmMessageFunction &func) const;
    AvailableModule transform(); //this invalidates this object;
};
typedef std::map<AvailableModule::Key, AvailableModule> AvailableMap;


} // namespace vistle
#endif
