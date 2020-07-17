#ifndef INSITU_MODULEINFO_H
#define INSITU_MODULEINFO_H

#include <string>
#include <set>

namespace vistle {
namespace insitu {
    struct ModuleInfo {
        bool initialized = false; //Module is initialized(sent port and command info)
        std::string shmName, name, numCons, hostname;
        int id = 0, port = 0;
        bool ready = false; //wether the module is executing or not
        std::set<std::string> connectedPorts;
    };
}//insitu
}//vistle

#endif