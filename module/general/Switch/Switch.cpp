#include "Switch.h"

MODULE_MAIN(Switch)

using namespace vistle;

Switch::Switch(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        m_inputs[i] = createInputPort("in" + std::to_string(i), "input " + std::to_string(i));
    }

    m_output = createOutputPort("out", "output");

    m_choice =
        addIntParameter("choice", "choose the input that is forwarded to output", 0, Parameter::Presentation::Choice);

    setParameterChoices(m_choice, {"0", "1", "2", "3"});
    configureParameter(m_choice, Parameter::RendererGui, 1);
}

void Switch::updateChoiceLabels()
{
    m_choiceToPort.clear();
    std::vector<std::string> choices;
    choices.reserve(NumPorts);
    m_choiceToPort.reserve(NumPorts);

    for (int i = 0; i < NumPorts; ++i) {
        const auto *port = m_inputs[i];
        if (!hasObject(port)) {
            continue;
        }

        const auto &obj = port->objects().front();

        std::string species;
        if (obj) {
            species = obj->getAttribute(attribute::Species);
        }
        if (species.empty()) {
            species = std::to_string(i);
        }

        choices.push_back(species);
        m_choiceToPort.push_back(i);
    }

    if (choices.empty()) {
        setParameterChoices(m_choice, {"(none available)"});
        setParameter(m_choice, Integer(0));
        return;
    }

    setParameterChoices(m_choice, choices);

    const int choice = m_choice->getValue();
    if (choice < 0 || choice >= static_cast<int>(m_choiceToPort.size())) {
        setParameter(m_choice, Integer(0));
    }
}

bool Switch::compute()
{
    updateChoiceLabels();

    if (m_choiceToPort.empty()) {
        sendError("no object on any input port");
        return false;
    }

    const int choice = m_choice->getValue();
    if (choice < 0 || choice >= static_cast<int>(m_choiceToPort.size())) {
        sendError("choice out of range");
        return false;
    }

    auto in = m_inputs[m_choiceToPort[choice]];
    Object::const_ptr data = expect<Object>(in);
    if (!data) {
        sendError("no object on input port");
        return false;
    }
    auto ndata = data->clone();
    updateMeta(ndata);
    addObject(m_output, ndata);
    return true;
}
