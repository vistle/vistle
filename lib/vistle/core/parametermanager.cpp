#include "parametermanager.h"
#include "messages.h"

#include <iostream>

#define CERR std::cerr << m_name << "_" << id() << " "

namespace vistle {

template<class Payload>
void ParameterManager::sendParameterMessageWithPayload(message::Message &message, Payload &payload)
{
    auto pl = addPayload(message, payload);
    return sendParameterMessage(message, &pl);
}

ParameterManager::ParameterManager(const std::string &name, int id): m_id(id), m_name(name)
{}

ParameterManager::~ParameterManager()
{
    if (!parameters.empty()) {
        CERR << parameters.size() << " not yet removed and quitting" << std::endl;
    }
}

void ParameterManager::setCurrentParameterGroup(const std::string &group, bool defaultExpanded)
{
    m_currentParameterGroup = group;
    m_currentParameterGroupExpanded = defaultExpanded;
}

void ParameterManager::init()
{
    for (auto &pair: parameters) {
#if 0
      parameterChangedWrapper(pair.second.get());
#else
        m_delayedChanges.push_back(pair.second.get());
#endif
    }
}

void ParameterManager::quit()
{
    std::vector<vistle::Parameter *> toRemove;
    for (auto &param: parameters) {
        toRemove.push_back(param.second.get());
    }
    for (auto &param: toRemove) {
        removeParameter(param);
    }
}

bool ParameterManager::handleMessage(const message::SetParameter &param)
{
    // sent by controller
    switch (param.getParameterType()) {
    case Parameter::Invalid:
        applyDelayedChanges();
        break;
    case Parameter::Integer:
        setIntParameter(param.getName(), param.getInteger(), &param);
        break;
    case Parameter::Float:
        setFloatParameter(param.getName(), param.getFloat(), &param);
        break;
    case Parameter::Vector:
        setVectorParameter(param.getName(), param.getVector(), &param);
        break;
    case Parameter::IntVector:
        setIntVectorParameter(param.getName(), param.getIntVector(), &param);
        break;
    case Parameter::String:
        setStringParameter(param.getName(), param.getString(), &param);
        break;
    default:
        CERR << "handleMessage: unknown parameter type " << param.getParameterType() << std::endl;
        assert("unknown parameter type" == 0);
        break;
    }

    return true;
}

bool ParameterManager::changeParameters(std::set<const Parameter *> params)
{
    bool ret = true;
    for (auto p: params)
        ret &= changeParameter(p);
    return ret;
}

const std::string &ParameterManager::currentParameterGroup() const
{
    return m_currentParameterGroup;
}

void ParameterManager::setId(int id)
{
    m_id = id;
}

int ParameterManager::id() const
{
    return m_id;
}

void ParameterManager::setName(const std::string &name)
{
    m_name = name;
}

void ParameterManager::applyDelayedChanges()
{
    std::set<const Parameter *> params;
    for (auto p: m_delayedChanges) {
        params.emplace(p);
    }
    m_delayedChanges.clear();

    changeParameters(params);
}

Parameter *ParameterManager::addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> param)
{
    parameters[name] = param;

    message::AddParameter add(*param, m_name);
    add.setDestId(message::Id::ForBroadcast);
    sendParameterMessage(add);

    message::SetParameter set(m_id, name, param, Parameter::Value, true);
    set.setDestId(message::Id::ForBroadcast);
    set.setReferrer(add.uuid());
    sendParameterMessage(set);

    return param.get();
}

bool ParameterManager::removeParameter(Parameter *param)
{
    auto it = parameters.find(param->getName());
    if (it == parameters.end()) {
        CERR << "removeParameter: no parameter with name " << param->getName() << std::endl;
        return false;
    }

    message::RemoveParameter remove(*param, m_name);
    remove.setDestId(message::Id::ForBroadcast);
    sendParameterMessage(remove);

    parameters.erase(it);

    return true;
}

bool ParameterManager::updateParameter(const std::string &name, const Parameter *param,
                                       const message::SetParameter *inResponseTo, Parameter::RangeType rt)
{
    auto i = parameters.find(name);

    if (i == parameters.end()) {
        CERR << "setParameter: " << name << " not found" << std::endl;
        return false;
    }

    if (i->second->type() != param->type()) {
        CERR << "setParameter: type mismatch for " << name << " " << i->second->type() << " != " << param->type()
             << std::endl;
        return false;
    }

    if (i->second.get() != param) {
        CERR << "setParameter: pointer mismatch for " << name << std::endl;
        return false;
    }

    message::SetParameter set(m_id, name, i->second, rt);
    if (inResponseTo) {
        set.setReferrer(inResponseTo->uuid());
        if (inResponseTo->isDelayed())
            set.setDelayed();
    }
    set.setDestId(message::Id::ForBroadcast);
    sendParameterMessage(set);

    return true;
}

void ParameterManager::setParameterChoices(const std::string &name, const std::vector<std::string> &choices)
{
    auto p = findParameter(name);
    if (p)
        setParameterChoices(p.get(), choices);
}

void ParameterManager::setParameterChoices(Parameter *param, const std::vector<std::string> &choices)
{
    message::SetParameterChoices sc(param->getName(), choices.size());
    sc.setDestId(message::Id::ForBroadcast);
    message::SetParameterChoices::Payload pl(choices);
    sendParameterMessageWithPayload(sc, pl);
}

void ParameterManager::setParameterFilters(const std::string &name, const std::string &filters)
{
    auto p = findParameter(name);
    auto sp = dynamic_cast<StringParameter *>(p.get());
    assert(sp);
    if (sp)
        setParameterFilters(sp, filters);
}

void ParameterManager::setParameterFilters(StringParameter *param, const std::string &filters)
{
    // abuse Minimum for filters
    param->setMinimum(filters);
    updateParameter(param->getName(), param, nullptr, Parameter::Minimum);
}

template<class T>
Parameter *ParameterManager::addParameter(const std::string &name, const std::string &description, const T &value,
                                          Parameter::Presentation pres)
{
    std::shared_ptr<Parameter> p(new ParameterBase<T>(id(), name, value));
    p->setDescription(description);
    p->setGroup(currentParameterGroup());
    p->setGroupExpanded(m_currentParameterGroupExpanded);
    p->setPresentation(pres);

    return addParameterGeneric(name, p);
}

std::shared_ptr<Parameter> ParameterManager::findParameter(const std::string &name) const
{
    auto i = parameters.find(name);

    if (i == parameters.end())
        return std::shared_ptr<Parameter>();

    return i->second;
}

StringParameter *ParameterManager::addStringParameter(const std::string &name, const std::string &description,
                                                      const std::string &value, Parameter::Presentation p)
{
    return dynamic_cast<StringParameter *>(addParameter(name, description, value, p));
}

bool ParameterManager::setStringParameter(const std::string &name, const std::string &value,
                                          const message::SetParameter *inResponseTo)
{
    return setParameter(name, value, inResponseTo);
}

std::string ParameterManager::getStringParameter(const std::string &name) const
{
    std::string value = "";
    getParameter(name, value);
    return value;
}

FloatParameter *ParameterManager::addFloatParameter(const std::string &name, const std::string &description,
                                                    const Float value)
{
    return dynamic_cast<FloatParameter *>(addParameter(name, description, value));
}

bool ParameterManager::setFloatParameter(const std::string &name, const Float value,
                                         const message::SetParameter *inResponseTo)
{
    return setParameter(name, value, inResponseTo);
}

Float ParameterManager::getFloatParameter(const std::string &name) const
{
    Float value = 0.;
    getParameter(name, value);
    return value;
}

IntParameter *ParameterManager::addIntParameter(const std::string &name, const std::string &description,
                                                const Integer value, Parameter::Presentation p)
{
    return dynamic_cast<IntParameter *>(addParameter(name, description, value, p));
}

bool ParameterManager::setIntParameter(const std::string &name, Integer value,
                                       const message::SetParameter *inResponseTo)
{
    return setParameter(name, value, inResponseTo);
}

Integer ParameterManager::getIntParameter(const std::string &name) const
{
    Integer value = 0;
    getParameter(name, value);
    return value;
}

VectorParameter *ParameterManager::addVectorParameter(const std::string &name, const std::string &description,
                                                      const ParamVector &value)
{
    return dynamic_cast<VectorParameter *>(addParameter(name, description, value));
}

bool ParameterManager::setVectorParameter(const std::string &name, const ParamVector &value,
                                          const message::SetParameter *inResponseTo)
{
    return setParameter(name, value, inResponseTo);
}

ParamVector ParameterManager::getVectorParameter(const std::string &name) const
{
    ParamVector value;
    getParameter(name, value);
    return value;
}

IntVectorParameter *ParameterManager::addIntVectorParameter(const std::string &name, const std::string &description,
                                                            const IntParamVector &value)
{
    return dynamic_cast<IntVectorParameter *>(addParameter(name, description, value));
}

bool ParameterManager::setIntVectorParameter(const std::string &name, const IntParamVector &value,
                                             const message::SetParameter *inResponseTo)
{
    return setParameter(name, value, inResponseTo);
}

IntParamVector ParameterManager::getIntVectorParameter(const std::string &name) const
{
    IntParamVector value;
    getParameter(name, value);
    return value;
}

bool ParameterManager::setParameterImmediate(Parameter *param, bool immed)
{
    param->setImmediate(immed);
    return updateParameter(param->getName(), param, nullptr, Parameter::Other);
}

bool ParameterManager::setParameterImmediate(const std::string &name, bool immed)
{
    auto param = findParameter(name);
    assert(param);
    if (param)
        return setParameterImmediate(param.get(), immed);
    return false;
}

bool ParameterManager::parameterChangedWrapper(const Parameter *p)
{
    if (m_inParameterChanged) {
        return true;
    }
    m_inParameterChanged = true;

    applyDelayedChanges();

    bool ret = changeParameter(p);
    m_inParameterChanged = false;
    return ret;
}

bool ParameterManager::changeParameter(const Parameter *p)
{
    return true;
}

} // namespace vistle
