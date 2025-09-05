#include "parametermanager.h"
#include "messages.h"
#include <vistle/config/access.h>
#include <vistle/config/value.h>
#include <vistle/config/array.h>

#include <iostream>
#include <regex>

#include "parametermanager_impl.h"

#define CERR std::cerr << message::Id::name(m_name, m_id) << ": "

namespace vistle {

template<class Payload>
void ParameterManager::sendParameterMessageWithPayload(message::Message &message, Payload &payload)
{
    auto pl = addPayload(message, payload);
    return sendParameterMessage(message, &pl);
}

int ParameterManager::parameterTargetModule(int id, const std::string &name)
{
    if (id != message::Id::Vistle) {
        return id;
    }

    auto pat = std::regex("\\[([0-9]+)\\]$");
    std::sregex_iterator it(name.begin(), name.end(), pat), end;
    for (; it != end; ++it) {
        const std::smatch &match = *it;
        const std::string str = match.str(1);
        int num = atol(str.c_str());
        return num;
    }

    return id;
}

ParameterManager::ParameterManager(const std::string &name, int id): m_id(id), m_name(name)
{}

ParameterManager::~ParameterManager()
{
    if (!m_parameters.empty()) {
        CERR << m_parameters.size() << " not yet removed and quitting" << std::endl;
    }
}

void ParameterManager::setConfig(config::Access *config)
{
    m_config = config;
}

void ParameterManager::setCurrentParameterGroup(const std::string &group, bool defaultExpanded)
{
    m_currentParameterGroup = group;
    m_currentParameterGroupExpanded = defaultExpanded;
}

void ParameterManager::init()
{
    for (auto &pair: m_parameters) {
        m_delayedChanges.emplace(pair.first, pair.second.param.get());
    }
}

void ParameterManager::quit()
{
    std::vector<std::string> toRemove;
    toRemove.reserve(m_parameters.size());
    for (auto &param: m_parameters) {
        if (param.second.owner)
            toRemove.push_back(param.second.param->getName());
    }
    for (auto &param: toRemove) {
        removeParameter(param);
    }
    m_parameters.clear();
}

bool ParameterManager::handleMessage(const message::AddParameter &add)
{
    if (add.destId() != id())
        return false;
    if (id() != message::Id::Config)
        return false;

    const auto *name = add.getName();
    const auto *desc = add.description();
    const auto pres = static_cast<Parameter::Presentation>(add.getPresentation());
    Parameter *param = nullptr;
    switch (add.getParameterType()) {
    case Parameter::Invalid:
        break;
    case Parameter::Integer:
        param = addParameter<Integer>(name, desc, Integer(), pres);
        break;
    case Parameter::Float:
        param = addParameter<Float>(name, desc, Float(), pres);
        break;
    case Parameter::Vector:
        param = addParameter(name, desc, ParamVector(), pres);
        break;
    case Parameter::IntVector:
        param = addParameter(name, desc, IntParamVector(), pres);
        break;
    case Parameter::String:
        param = addParameter(name, desc, std::string(), pres);
        break;
    case Parameter::StringVector:
        param = addParameter(name, desc, StringParamVector(), pres);
        break;
    default:
        CERR << "handleMessage: unknown parameter type " << add.getParameterType() << std::endl;
        assert("unknown parameter type" == 0);
        return false;
        break;
    }

    if (param) {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end()) {
            it->second.owner = false;
        }
    }

    return true;
}

bool ParameterManager::handleMessage(const message::RemoveParameter &remove)
{
    if (remove.destId() != id())
        return false;
    if (id() != message::Id::Config)
        return false;

    const auto *name = remove.getName();
    return removeParameter(name);
}

bool ParameterManager::handleMessage(const message::SetParameter &param)
{
    if (param.destId() != id())
        return false;

    // sent by controller
    bool handled = false;
    switch (param.getParameterType()) {
    case Parameter::Invalid:
        applyDelayedChanges();
        handled = true;
        break;
    case Parameter::Integer:
        handled = setIntParameter(param.getName(), param.getInteger(), &param);
        break;
    case Parameter::Float:
        handled = setFloatParameter(param.getName(), param.getFloat(), &param);
        break;
    case Parameter::Vector:
        handled = setVectorParameter(param.getName(), param.getVector(), &param);
        break;
    case Parameter::IntVector:
        handled = setIntVectorParameter(param.getName(), param.getIntVector(), &param);
        break;
    case Parameter::String:
        handled = setStringParameter(param.getName(), param.getString(), &param);
        break;
    case Parameter::StringVector:
        handled = setStringVectorParameter(param.getName(), param.getStringVector(), &param);
        break;
    default:
        CERR << "handleMessage: unknown parameter type " << param.getParameterType() << std::endl;
        assert("unknown parameter type" == 0);
        return false;
    }

    if (!handled) {
        //CERR << "queuing " << param << std::endl;
        m_queue.emplace_back(param);
    }

    return handled;
}

bool ParameterManager::changeParameters(std::map<std::string, const Parameter *> params)
{
    bool ret = true;
    for (auto p: params)
        ret &= changeParameter(p.second);
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

bool ParameterManager::applyDelayedChanges()
{
    decltype(m_delayedChanges) delayedChanges;
    std::swap(m_delayedChanges, delayedChanges);
    if (m_inApplyDelayedChanges) {
        if (delayedChanges.empty()) {
            return true;
        }
        return changeParameters(delayedChanges);
    }

    m_inApplyDelayedChanges = true;
    bool ret = changeParameters(delayedChanges);
    m_inApplyDelayedChanges = false;
    return ret;
}

Parameter *ParameterManager::addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> param)
{
    m_parameters[name] = param;

    message::AddParameter add(*param, m_name);
    add.setDestId(message::Id::ForBroadcast);
    sendParameterMessage(add);

    message::SetParameter set(m_id, name, param, Parameter::Value, true);
    set.setDestId(message::Id::ForBroadcast);
    set.setReferrer(add.uuid());
    sendParameterMessage(set);

    return param.get();
}

bool ParameterManager::removeParameter(const std::string &name)
{
    auto it = m_parameters.find(name);
    if (it == m_parameters.end()) {
        CERR << "removeParameter: no parameter with name " << name << std::endl;
        return false;
    }

    if (it->second.owner) {
        message::RemoveParameter remove(*it->second.param, m_name);
        remove.setDestId(message::Id::ForBroadcast);
        sendParameterMessage(remove);
    } else {
        CERR << "removeParameter not owner: " << name << std::endl;
    }

    it = m_parameters.find(name);
    if (it == m_parameters.end()) {
        CERR << "removeParameter: parameter with name " << name << " disappeared while being removed" << std::endl;
        return false;
    }
    m_parameters.erase(it);

    return true;
}

bool ParameterManager::updateParameter(const std::string &name, const Parameter *param,
                                       const message::SetParameter *inResponseTo, Parameter::RangeType rt)
{
    auto i = m_parameters.find(name);

    if (i == m_parameters.end()) {
        CERR << "setParameter: " << name << " not found" << std::endl;
        return false;
    }

    if (i->second.param->type() != param->type()) {
        CERR << "setParameter: type mismatch for " << name << " " << i->second.param->type() << " != " << param->type()
             << std::endl;
        return false;
    }

    if (i->second.param.get() != param) {
        CERR << "setParameter: pointer mismatch for " << name << std::endl;
        return false;
    }

    message::SetParameter set(m_id, name, i->second.param, rt);
    if (inResponseTo) {
        if (!i->second.owner)
            return true;
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
    param->setChoices(choices);
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

template<class V>
V getParameterDefault(config::Access *config, const std::string &module, const std::string &name, const V &value)
{
    if (!config)
        return value;
    if (name.find("_config:") == 0) // avoid recursive default look-up for config parameters
        return value;

    auto def = config->value<V>("modules/parameters", module, name, value);
    if (!name.empty() && name[0] == '_') {
        if (!def->exists()) {
            // look up defaults for system parameters
            def = config->value<V>("modules/default", module, name, value);
        }
        if (!def->exists()) {
            // global user overrides for system parameters
            def = config->value<V>("modules/parameters", "ALL", name, value);
        }
        if (!def->exists()) {
            // look up common default for system parameters
            def = config->value<V>("modules/default", "ALL", name, value);
        }
    }
    return def->value();
}

template<class V>
ParameterVector<V> getParameterDefault(config::Access *config, const std::string &module, const std::string &name,
                                       const ParameterVector<V> &value)
{
    if (!config)
        return value;
    if (name.find("_config:") == 0)
        return value;

    auto def = config->array<V>("modules/default", module, name);
    if (!def->exists() && !name.empty() && name[0] == '_') {
        // look up common default for system parameters
        def = config->array<V>("modules/default", "ALL", name);
    }
    if (def->size() != value.size())
        return value;

    ParameterVector<V> val;
    val.reserve(def->size());
    for (size_t i = 0; i < def->size(); ++i) {
        val.push_back((*def)[i]);
    }
    return val;
}

template<class T>
Parameter *ParameterManager::addParameter(const std::string &name, const std::string &description, const T &value,
                                          Parameter::Presentation pres)
{
    auto def = getParameterDefault(m_config, m_name, name, value);
    std::shared_ptr<Parameter> p(new ParameterBase<T>(id(), name, def));
    p->setDescription(description);
    p->setGroup(currentParameterGroup());
    p->setGroupExpanded(m_currentParameterGroupExpanded);
    p->setPresentation(pres);

    auto param = addParameterGeneric(name, p);

    std::deque<message::SetParameter> q;
    std::swap(m_queue, q);
    for (auto &m: q) {
        //CERR << "retrying " << m << std::endl;
        handleMessage(m);
    }

    return param;
}

std::shared_ptr<Parameter> ParameterManager::findParameter(const std::string &name) const
{
    auto it = m_parameters.find(name);

    if (it == m_parameters.end())
        return std::shared_ptr<Parameter>();

    return it->second.param;
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
                                                    const Float value, Parameter::Presentation p)
{
    return dynamic_cast<FloatParameter *>(addParameter(name, description, value, p));
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
                                                      ParamVector value, Parameter::Presentation p)
{
    return dynamic_cast<VectorParameter *>(addParameter(name, description, value, p));
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
                                                            IntParamVector value, Parameter::Presentation p)
{
    return dynamic_cast<IntVectorParameter *>(addParameter(name, description, value, p));
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

StringVectorParameter *ParameterManager::addStringVectorParameter(const std::string &name,
                                                                  const std::string &description,
                                                                  StringParamVector value, Parameter::Presentation p)
{
    return dynamic_cast<StringVectorParameter *>(addParameter(name, description, value, p));
}

bool ParameterManager::setStringVectorParameter(const std::string &name, const StringParamVector &value,
                                                const message::SetParameter *inResponseTo)
{
    return setParameter(name, value, inResponseTo);
}

StringParamVector ParameterManager::getStringVectorParameter(const std::string &name) const
{
    StringParamVector value;
    getParameter(name, value);
    return value;
}

bool ParameterManager::setParameterReadOnly(Parameter *param, bool readOnly)
{
    if (!param)
        return false;
    param->setReadOnly(readOnly);
    return updateParameter(param->getName(), param, nullptr, Parameter::Other);
}

bool ParameterManager::setParameterReadOnly(const std::string &name, bool readOnly)
{
    auto param = findParameter(name);
    assert(param);
    if (param)
        return setParameterReadOnly(param.get(), readOnly);
    return false;
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

    bool ret = changeParameter(p);
    m_inParameterChanged = false;
    return ret;
}

bool ParameterManager::changeParameter(const Parameter *p)
{
    return true;
}

PARAM_TYPE_TEMPLATE(, Integer)
PARAM_TYPE_TEMPLATE(, Float)
PARAM_TYPE_TEMPLATE(, std::string)
PARAM_TYPE_TEMPLATE(, ParameterVector<Integer>)
PARAM_TYPE_TEMPLATE(, ParameterVector<Float>)
PARAM_TYPE_TEMPLATE(, ParameterVector<std::string>)

} // namespace vistle
