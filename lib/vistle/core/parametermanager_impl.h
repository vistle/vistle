#ifndef VISTLE_PARAMETER_MANAGER_IMPL_H
#define VISTLE_PARAMETER_MANAGER_IMPL_H

#include <cassert>

namespace vistle {

template<class T>
bool ParameterManager::setParameter(const std::string &name, const T &value, const message::SetParameter *inResponseTo)
{
    auto param = findParameter(name);
    if (!param) {
        std::cerr << "ParameterManager: parameter " << name << " not found" << std::endl;
        return false;
    }
    auto p = std::dynamic_pointer_cast<ParameterBase<T>>(param);
    if (!p) {
        std::cerr << "ParameterManager: parameter " << name << " not of requested type, value=" << value << std::endl;
        return false;
    }

    return setParameter(p.get(), value, inResponseTo);
}

template<class T>
bool ParameterManager::setParameter(ParameterBase<T> *param, const T &value, const message::SetParameter *inResponseTo)
{
    bool delayed = false;
    if (inResponseTo && inResponseTo->isDelayed())
        delayed = true;
    param->setValue(value, false, delayed);
    if (!inResponseTo || !inResponseTo->isDelayed())
        parameterChangedWrapper(param);
    else
        m_delayedChanges.push_back(param);
    return updateParameter(param->getName(), param, inResponseTo);
}

template<class T>
bool ParameterManager::setParameterMinimum(ParameterBase<T> *param, const T &minimum)
{
    return ParameterManager::setParameterRange(param, minimum, param->maximum());
}

template<class T>
bool ParameterManager::setParameterMaximum(ParameterBase<T> *param, const T &maximum)
{
    return ParameterManager::setParameterRange(param, param->minimum(), maximum);
}

template<class T>
bool ParameterManager::setParameterRange(const std::string &name, const T &minimum, const T &maximum)
{
    auto p = std::dynamic_pointer_cast<ParameterBase<T>>(findParameter(name));
    if (!p)
        return false;

    return setParameterRange(p.get(), minimum, maximum);
}

template<class T>
bool ParameterManager::setParameterRange(ParameterBase<T> *param, const T &minimum, const T &maximum)
{
    bool ok = true;

    if (minimum > maximum) {
        std::cerr << "range for parameter " << param->getName() << " swapped: min " << minimum << " > max " << maximum
                  << std::endl;
        param->setMinimum(maximum);
        param->setMaximum(minimum);
    } else {
        param->setMinimum(minimum);
        param->setMaximum(maximum);
    }
    T value = param->getValue();
    if (value < param->minimum()) {
        std::cerr << "value " << value << " for parameter " << param->getName()
                  << " increased to minimum: " << param->minimum() << std::endl;
        param->setValue(param->minimum());
        ok &= updateParameter(param->getName(), param, nullptr);
    }
    if (value > param->maximum()) {
        std::cerr << "value " << value << " for parameter " << param->getName()
                  << " decreased to maximum: " << param->maximum() << std::endl;
        param->setValue(param->maximum());
        ok &= updateParameter(param->getName(), param, nullptr);
    }
    ok &= updateParameter(param->getName(), param, nullptr, Parameter::Minimum);
    ok &= updateParameter(param->getName(), param, nullptr, Parameter::Maximum);
    return ok;
}

template<class T>
bool ParameterManager::getParameter(const std::string &name, T &value) const
{
    if (auto p = std::dynamic_pointer_cast<ParameterBase<T>>(findParameter(name))) {
        value = p->getValue();
    } else {
        std::cerr << "ParameterManager::getParameter(" << name << "): type failure" << std::endl;
        assert("dynamic_cast failed" == 0);
        return false;
    }

    return true;
}

} // namespace vistle
#endif
