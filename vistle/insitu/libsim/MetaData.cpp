#include "MetaData.h"
#include "Exception.h"

#include <vistle/insitu/libsim/libsimInterface/CommandMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/CurveMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/ExpressionMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/MaterialMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/MeshMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/MessageMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/SimulationMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/SpeciesMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/VariableMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

#include <vistle/insitu/message/InSituMessage.h>

using namespace vistle::insitu::libsim;

//...............get meta date.......................................................................

bool MetaData::timestepChanged() const
{
    return m_timestepChanged;
}

int MetaData::simMode() const
{
    return m_simMode;
}

size_t MetaData::currentCycle() const
{
    return static_cast<size_t>(m_currentCycle);
}

double MetaData::currentTime() const
{
    return m_currentTime;
}

visit_handle MetaData::handle() const
{
    return m_handle;
}

void MetaData::fetchFromSim()
{
    m_handle = simv2_invoke_GetMetaData(); //somehow 0 is valid
    v2check(simv2_SimulationMetaData_getData, m_handle, m_simMode, m_currentCycle, m_currentTime);
}

size_t MetaData::getNumObjects(SimulationObjectType type) const
{
    std::function<int(visit_handle, int &)> getNum;
    switch (type) {
    case SimulationObjectType::mesh: {
        getNum = simv2_SimulationMetaData_getNumMeshes;
    } break;
    case SimulationObjectType::variable: {
        getNum = simv2_SimulationMetaData_getNumVariables;
    } break;
    case SimulationObjectType::material: {
        getNum = simv2_SimulationMetaData_getNumMaterials;
    } break;
    case SimulationObjectType::curve: {
        getNum = simv2_SimulationMetaData_getNumCurves;
    } break;
    case SimulationObjectType::expression: {
        getNum = simv2_SimulationMetaData_getNumExpressions;
    } break;
    case SimulationObjectType::species: {
        getNum = simv2_SimulationMetaData_getNumSpecies;
    } break;
    case SimulationObjectType::genericCommand: {
        getNum = simv2_SimulationMetaData_getNumGenericCommands;
    } break;
    case SimulationObjectType::customCommand: {
        getNum = simv2_SimulationMetaData_getNumCustomCommands;
    } break;
    case SimulationObjectType::message: {
        getNum = simv2_SimulationMetaData_getNumMessages;
    } break;
    default:
        throw EngineException("getDataNames called with invalid type") << (int)type;

        break;
    }

    int num = 0;
    v2check(getNum, m_handle, num);
    assert(num >= 0);
    return static_cast<size_t>(num);
}

visit_handle MetaData::getNthObject(SimulationObjectType type, int n) const
{
    std::function<int(visit_handle, int, visit_handle &)> getObj;
    switch (type) {
    case SimulationObjectType::mesh: {
        getObj = simv2_SimulationMetaData_getMesh;
    } break;
    case SimulationObjectType::variable: {
        getObj = simv2_SimulationMetaData_getVariable;
    } break;
    case SimulationObjectType::material: {
        getObj = simv2_SimulationMetaData_getMaterial;
    } break;
    case SimulationObjectType::curve: {
        getObj = simv2_SimulationMetaData_getCurve;
    } break;
    case SimulationObjectType::expression: {
        getObj = simv2_SimulationMetaData_getExpression;
    } break;
    case SimulationObjectType::species: {
        getObj = simv2_SimulationMetaData_getSpecies;
    } break;
    case SimulationObjectType::genericCommand: {
        getObj = simv2_SimulationMetaData_getGenericCommand;
    } break;
    case SimulationObjectType::customCommand: {
        getObj = simv2_SimulationMetaData_getCustomCommand;
    } break;
    case SimulationObjectType::message: {
        getObj = simv2_SimulationMetaData_getMessage;
    } break;
    default:
        throw EngineException("getDataNames called with invalid type");
        break;
    }
    visit_handle obj = m_handle;
    v2check(getObj, obj, n, obj);
    return obj;
}

std::vector<std::string> MetaData::getObjectNames(SimulationObjectType type) const
{
    std::function<int(visit_handle, char **)> getName;
    switch (type) {
    case SimulationObjectType::mesh: {
        getName = simv2_MeshMetaData_getName;
    } break;
    case SimulationObjectType::variable: {
        getName = simv2_VariableMetaData_getName;
    } break;
    case SimulationObjectType::material: {
        getName = simv2_MaterialMetaData_getName;
    } break;
    case SimulationObjectType::curve: {
        getName = simv2_CurveMetaData_getName;
    } break;
    case SimulationObjectType::expression: {
        getName = simv2_ExpressionMetaData_getName;
    } break;
    case SimulationObjectType::species: {
        getName = simv2_SpeciesMetaData_getName;
    } break;
    case SimulationObjectType::genericCommand: {
        getName = simv2_CommandMetaData_getName;
    } break;
    case SimulationObjectType::customCommand: {
        getName = simv2_CommandMetaData_getName;
    } break;
    case SimulationObjectType::message: {
        getName = simv2_MessageMetaData_getName;
    } break;
    default:
        throw EngineException("getDataNames called with invalid type");
        break;
    }

    auto n = getNumObjects(type);
    std::vector<std::string> names;
    names.reserve(n);
    for (size_t i = 0; i < n; i++) {
        visit_handle obj = getNthObject(type, i);
        char *name;
        v2check(getName, obj, &name);
        names.push_back(name);
        free(name);
    }
    return names;
}

std::vector<std::string> MetaData::getRegisteredCommands(SimulationObjectType type) const
{
    std::vector<std::string> commands;
    for (size_t i = 0; i < getNumObjects(type); i++) {
        visit_handle commandHandle = getNthObject(type, i);
        char *name;
        v2check(simv2_CommandMetaData_getName, commandHandle, &name);
        commands.push_back(name);
        free(name);
    }
    return commands;
}

std::vector<std::string> MetaData::getRegisteredGenericCommands() const
{
    return getRegisteredCommands(SimulationObjectType::genericCommand);
}

std::vector<std::string> MetaData::getRegisteredCustomCommands() const
{
    return getRegisteredCommands(SimulationObjectType::customCommand);
}

std::vector<std::vector<std::string>> MetaData::getMeshAndFieldNames() const
{
    std::vector<std::vector<std::string>> ports;
    std::vector<std::string> meshNames = getObjectNames(SimulationObjectType::mesh);
    meshNames.push_back("mesh");
    ports.push_back(meshNames);

    auto varNames = getObjectNames(SimulationObjectType::variable);
    varNames.push_back("variable");
    ports.push_back(varNames);

    return ports;
}

std::string MetaData::getName(const visit_handle &handle, SimulationObjectType type) const
{
    char *name;

    switch (type) {
    case SimulationObjectType::mesh:
        v2check(simv2_MeshMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::variable:
        v2check(simv2_VariableMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::material:
        v2check(simv2_MaterialMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::curve:
        v2check(simv2_CurveMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::expression:
        v2check(simv2_ExpressionMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::species:
        v2check(simv2_SpeciesMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::genericCommand:
        v2check(simv2_CommandMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::customCommand:
        v2check(simv2_CommandMetaData_getName, handle, &name);
        break;
    case SimulationObjectType::message:
        v2check(simv2_MessageMetaData_getName, handle, &name);
        break;
    default:
        break;
    }
    std::string n{name};
    free(name);
    return n;
}
//...................................................................................................
