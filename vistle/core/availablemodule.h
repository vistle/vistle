#ifndef VISTLE_AVAILABLEMODULE_H
#define VISTLE_AVAILABLEMODULE_H

#include <string>
#include <map>
#include "export.h"

namespace vistle {

const int ModuleNameLength = 50;

struct V_COREEXPORT AvailableModule {
    struct Key {
        int hub;
        std::string name;

        Key(int hub, const std::string &name);

        bool operator<(const Key &rhs) const;
    };

    int hub;
    std::string name;
    std::string path;
    std::string description;

    AvailableModule();
};

typedef std::map<AvailableModule::Key, AvailableModule> AvailableMap;

}
#endif
