#ifndef VISTLE_AVAILABLEMODULE_H
#define VISTLE_AVAILABLEMODULE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "export.h"
#include "object.h"

namespace vistle {

const int ModuleNameLength = 50;

struct V_COREEXPORT AvailableModule {
    struct Key {
        int hub;
        std::string name;

        Key(int hub, const std::string &name);

        bool operator<(const Key &rhs) const;
    };

    int hub = 0;
    std::string name;
    std::string path;
    std::string description;
    AvailableModule();
    void print() const;
    //for module compounds
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
    std::set<Connection> connections; //maps input-, outputports and params to parent ones
    std::vector<SubModule> submodules;
    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &description;
        ar &connections;
        ar &submodules;
    }
};
const std::string ModulePresetExtension = ".comp";
typedef std::map<AvailableModule::Key, AvailableModule> AvailableMap;

} // namespace vistle
#endif
