#include "availablemodule.h"

namespace vistle {

AvailableModule::AvailableModule()
: hub(0)
{}

AvailableModule::Key::Key(int hub, const std::string &name)
: hub(hub)
, name(name)
{}

bool AvailableModule::Key::operator<(const Key &rhs) const {
    if (hub == rhs.hub) {
        return name < rhs.name;
    }
    return hub < rhs.hub;
}

}

