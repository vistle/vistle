#ifndef INSITU_MODULEINFO_H
#define INSITU_MODULEINFO_H

#include "export.h"

#include <set>
#include <string>

#include <vistle/core/object.h>

namespace vistle {
namespace insitu {
namespace message {
class Message;
class V_INSITUMESSAGEEXPORT ModuleInfo {
public:
    struct ShmInfo {
        size_t id = 0, mpiSize = 0, numCons = 0;
        std::string shmName, name, hostname;
        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &id;
            ar &mpiSize;
            ar &shmName;
            ar &name;
            ar &numCons;
            ar &hostname;
        }
    };

    bool isInitialized() const;
    bool isReady() const;
    bool isPortConnected(const std::string &portName) const;
    size_t id() const;
    size_t mpiSize() const;
    std::string name() const;
    std::string hostname() const;
    std::string shmName() const;
    std::string uniqueSuffix() const; // iterator of connection attempts in string form to easily append to shm names

    bool update(const Message &msg); // returns if message was of a type concerning this class
    void update(const ModuleInfo::ShmInfo &shmInfo);
    void setInitState(bool state);
    void setReadyState(bool state);

private:
    bool m_initialized = false; // Module is initialized(sent port and command info)
    bool m_ready = false; // whether the module is executing or not
    std::set<std::string> m_connectedPorts;
    ShmInfo m_shmInfo;
};
} // namespace message
} // namespace insitu
} // namespace vistle

#endif
