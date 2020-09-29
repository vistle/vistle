#ifndef SIMULATION_META_DATA_H
#define SIMULATION_META_DATA_H

#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>
#include <vector>
#include <string>
namespace vistle {
namespace insitu {
namespace libsim {

enum class SimulationObjectType {
    mesh,
    variable,
    material,
    curve,
    expression,
    species,
    genericCommand,
    customCommand,
    message

};

class MetaData {
public:
    bool timestepChanged() const;
    int simMode() const;
    size_t currentCycle() const;
    double currentTime() const;
    const visit_handle &handle() const;

    //functions to retrieve meta data from sim

    void fetchFromSim(); //let the sim generate new meta data for the current cycle
    size_t getNumObjects(SimulationObjectType type) const;
    visit_handle getNthObject(SimulationObjectType type, int n) const;
    std::vector<std::string> getObjectNames(SimulationObjectType type) const;
    std::vector<std::string> getRegisteredGenericCommands() const;
    std::vector<std::vector<std::string>> getMeshAndFieldNames() const;
    const char *getName(const visit_handle &handle, SimulationObjectType type) const;

private:
    bool m_timestepChanged = false;
    int m_simMode = VISIT_SIMMODE_UNKNOWN;
    int m_currentCycle = 0;
    double m_currentTime = 0;
    visit_handle m_handle = VISIT_INVALID_HANDLE;
};
} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !SIMULATION_META_DATA_H
