#include "CoverConfigBridge.h"
#include <OpenConfig/array.h>
#include <OpenConfig/value.h>

using namespace vistle;

template<class V>
struct TypeMap {
    typedef V ValueType;
    typedef vistle::ParameterBase<V> ValueParam;
    typedef vistle::ParameterBase<vistle::ParameterVector<V>> ArrayParam;
    static const vistle::Parameter::Presentation presentation = vistle::Parameter::Generic;
};

template<>
struct TypeMap<bool> {
    typedef Integer ValueType;
    typedef IntParameter ValueParam;
    typedef IntVectorParameter ArrayParam;
    static const vistle::Parameter::Presentation presentation = vistle::Parameter::Boolean;
};

CoverConfigBridge::Key::Key() = default;
CoverConfigBridge::Key::Key(const opencover::config::ConfigBase *entry)
: path(entry->path()), section(entry->section()), name(entry->name())
{}
bool CoverConfigBridge::Key::operator<(const CoverConfigBridge::Key &o) const
{
    if (path == o.path) {
        if (section == o.section) {
            return name < o.name;
        }
        return section < o.section;
    }
    return path < o.path;
}

std::string CoverConfigBridge::Key::paramName() const
{
    std::string pname = path + ":" + section + ":" + name;
    return "_config:" + pname;
}


std::ostream &operator<<(std::ostream &os, const CoverConfigBridge::Key &key)
{
    os << key.path << ":" << key.section << ":" << key.name;
    return os;
}

namespace {
template<class V>
std::ostream &operator<<(std::ostream &os, const std::vector<V> &vec)
{
    os << "{";
    for (const auto &val: vec) {
        os << " " << val;
    }
    os << " }";
    return os;
}
} // namespace


CoverConfigBridge::CoverConfigBridge(COVER *cover): m_cover(cover)
{}

template<class V>
bool CoverConfigBridge::trySetParam(const opencover::config::ConfigBase *entry, const Key &key,
                                    vistle::Parameter *param)
{
    typedef typename TypeMap<V>::ValueType ValueType;
    typedef typename TypeMap<V>::ValueParam ValueParam;
    typedef typename TypeMap<V>::ArrayParam ArrayParam;
    auto presentation = TypeMap<V>::presentation;

    auto pname = key.paramName();
    if (const auto *value = dynamic_cast<const opencover::config::Value<V> *>(entry)) {
        auto pp = dynamic_cast<ValueParam *>(param);
        assert(!param || pp);
        if (!pp) {
            //std::cerr << "adding config value param " << pname << ", default=" << ValueType(value->defaultValue()) << std::endl;
            pp = dynamic_cast<ValueParam *>(
                m_cover->addParameter(pname, pname, ValueType(value->defaultValue()), presentation));
            m_params[key] = pp;
            m_values[pname] = key;
            if (value->defaultValue() == value->value())
                pp = nullptr;
        }
        if (pp) {
            //std::cerr << "storing config param " << pname << ", value=" << ValueType(value->value()) << std::endl;
            m_cover->setParameter(pname, ValueType(value->value()));
        }
        return true;
    }
    if (const auto *array = dynamic_cast<const opencover::config::Array<V> *>(entry)) {
        auto pp = dynamic_cast<ArrayParam *>(param);
        assert(!param || pp);
        if (!pp) {
            auto vec = array->defaultValue();
            std::vector<ValueType> vvec(vec.size());
            std::copy(vec.begin(), vec.end(), vvec.begin());
            ParameterVector<ValueType> def(vvec);
            //std::cerr << "adding config array param " << pname << ", default=" << def << std::endl;
            pp = dynamic_cast<ArrayParam *>(m_cover->addParameter(pname, pname, def, presentation));
            m_params[key] = pp;
            m_values[pname] = key;
            if (array->defaultValue() == array->value())
                pp = nullptr;
        }
        if (pp) {
            auto vec = array->value();
            std::vector<ValueType> vvec(vec.size());
            std::copy(vec.begin(), vec.end(), vvec.begin());
            ParameterVector<ValueType> val(vvec);
            //std::cerr << "storing config param " << pname << ", array=" << val << std::endl;
            m_cover->setParameter(pname, val);
        }
        return true;
    }
    return false;
}

bool CoverConfigBridge::wasChanged(const opencover::config::ConfigBase *entry)
{
    auto path = entry->path();
    auto section = entry->section();
    auto name = entry->name();
    Key key(entry);
    //std::cerr << "have to store " << key << std::endl;
    std::string pname = key.paramName();
    Parameter *param = nullptr;
    auto it = m_params.find(key);
    if (it != m_params.end()) {
        //std::cerr << "already have param " << pname << std::endl;
        param = it->second;
    }

    trySetParam<bool>(entry, key, param);
    trySetParam<int64_t>(entry, key, param);
    trySetParam<double>(entry, key, param);
    trySetParam<std::string>(entry, key, param);

    return true;
}

template<class V>
bool CoverConfigBridge::trySetValueOrArray(const Key &key, const Parameter *param)
{
    typedef typename TypeMap<V>::ValueParam ValueParam;
    typedef typename TypeMap<V>::ArrayParam ArrayParam;
    auto presentation = TypeMap<V>::presentation;

    auto pname = key.paramName();
    if (const auto *vparam = dynamic_cast<const ValueParam *>(param)) {
        if (vparam->presentation() == presentation) {
            if (auto val = m_cover->m_config->value<V>(key.path, key.section, key.name)) {
                *val = vparam->getValue();
                //std::cerr << "config value from Vistle: " << pname << " to " << val->value() << std::endl;
                return true;
            }
        }
    }
    if (const auto *aparam = dynamic_cast<const ArrayParam *>(param)) {
        if (aparam->presentation() == presentation) {
            if (auto arr = m_cover->m_config->array<V>(key.path, key.section, key.name)) {
                auto vp = aparam->getValue();
                std::vector<V> vec(vp.size());
                std::copy(vp.begin(), vp.end(), vec.begin());
                *arr = vec;
                //std::cerr << "config array from Vistle: " << pname << " to " << arr->value() << std::endl;
                return true;
            }
        }
    }
    return false;
}

bool CoverConfigBridge::changeParameter(const Parameter *param)
{
    auto pname = param->getName();
    auto it = m_values.find(pname);
    if (it == m_values.end()) {
        return false;
    }
    //std::cerr << "setting config param " << pname << std::endl;
    const auto &key = it->second;
    trySetValueOrArray<bool>(key, param);
    trySetValueOrArray<int64_t>(key, param);
    trySetValueOrArray<double>(key, param);
    trySetValueOrArray<std::string>(key, param);
    return true;
}
