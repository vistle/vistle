#include "Switch.h"
#include <vistle/core/statetracker.h>

MODULE_MAIN(Switch)

using namespace vistle;

std::vector<std::string> defaultSpecies()
{
    std::vector<std::string> species;
    for (int i = 0; i < NumPorts; ++i) {
        species.push_back("empty");
    }
    species.push_back("No output");
    return species;
}

Switch::Switch(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), m_inputSpecies(defaultSpecies())
{
    for (int i = 0; i < NumPorts; ++i) {
        m_inputs[i] = createInputPort("in" + std::to_string(i), "input " + std::to_string(i),
                                      i == 0 ? Port::Flags::NONE : Port::Flags::NOCOMPUTE);
    }

    m_output = createOutputPort("out", "output");

    m_choice =
        addIntParameter("choice", "choose the input that is forwarded to output", 0, Parameter::Presentation::Choice);


    setParameterChoices(m_choice, m_inputSpecies);
    configureParameter(m_choice, Parameter::RendererGui, 1);
}

bool Switch::changeParameter(const Parameter *p)
{
    if (p == m_choice) {
        const int choice = m_choice->getValue();
        for (int i = 0; i < NumPorts; ++i) {
            setPortFlags(m_inputs[i], i == choice ? Port::Flags::NONE : Port::Flags::NOCOMPUTE);
        }
    }
    return Module::changeParameter(p);
}

bool Switch::compute()
{
    // if choice == m_inputPorts.size(), compute is not called, so we don't have to handle that case here
    const int choice = m_choice->getValue();
    auto in = m_inputs[choice];
    Object::const_ptr data = expect<Object>(in);
    auto ndata = data->clone();
    updateMeta(ndata);
    addObject(m_output, ndata);

    return true;
}

bool Switch::objectAdded(int sender, const std::string &senderPort, const Port *port)
{
    size_t index = std::distance(m_inputs.begin(), std::find(m_inputs.begin(), m_inputs.end(), port));
    auto species = port->objects().back()->getAttribute(attribute::Species);
    auto datasetName = port->objects().back()->getAttribute(attribute::DatasetName);
    if (m_inputSpecies[index] != species) {
        m_inputSpecies[index] = datasetName.empty() ? species : datasetName + ":" + species;
        setParameterChoices(m_choice, m_inputSpecies);
    }

    return true;
}

void Switch::connectionAdded(const Port *from, const Port *to)
{
    auto name = state().getModuleName(from->getModuleID()) + "->" + from->getName() + ":empty";
    connectionChanged(to, name);
}

void Switch::connectionRemoved(const Port *from, const Port *to)
{
    connectionChanged(to, "empty");
}

void Switch::connectionChanged(const Port *to, const std::string &newDisplayName)
{
    auto it = std::find(m_inputs.begin(), m_inputs.end(), to);
    if (it != m_inputs.end()) {
        auto index = std::distance(m_inputs.begin(), it);
        m_inputSpecies[index] = newDisplayName;
        setParameterChoices(m_choice, m_inputSpecies);
    }
}
