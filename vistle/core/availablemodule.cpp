#include "availablemodule.h"

namespace vistle {

AvailableModule::AvailableModule(): hub(0)
{}

void AvailableModule::print() const
{
    std::cerr << "AvailableModule: " << name << " from hub" << hub << std::endl;
    if (!submodules.empty()) {
        std::cerr << "This is a module compund consisting of " << submodules.size() << " submodules:" << std::endl;

        for (const auto &conn: connections) {
            auto from = conn.fromId == -1 ? "compound" : submodules[conn.fromId].name;
            auto to = conn.toId == -1 ? "compound" : submodules[conn.toId].name;

            std::cerr << from << "'s port " << conn.fromPort << " is connected to " << to << "'s port " << conn.toPort
                      << std::endl;
        }
    }
}


AvailableModule::Key::Key(int hub, const std::string &name): hub(hub), name(name)
{}

bool AvailableModule::Key::operator<(const Key &rhs) const
{
    if (hub == rhs.hub) {
        return name < rhs.name;
    }
    return hub < rhs.hub;
}

} // namespace vistle
