#include "parameter.h"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>

namespace vistle {

#define V_PARAM_TYPE_INST(ValueType, Name) template class ParameterBase<ValueType>;

V_PARAM_TYPE_INST(ParamVector, VectorParameter)
V_PARAM_TYPE_INST(IntParamVector, IntVectorParameter)
V_PARAM_TYPE_INST(StringParamVector, StringVectorParameter)
V_PARAM_TYPE_INST(Float, FloatParameter)
V_PARAM_TYPE_INST(Integer, IntParameter)
V_PARAM_TYPE_INST(std::string, StringParameter)
#undef V_PARAM_TYPE_INST

Parameter::Parameter(int moduleId, const std::string &n, Parameter::Type type, Parameter::Presentation p)
: m_module(moduleId), m_name(n), m_type(type), m_presentation(p)
{}

Parameter::Parameter(const Parameter &other)
: m_choices(other.m_choices)
, m_module(other.m_module)
, m_name(other.m_name)
, m_description(other.m_description)
, m_type(other.m_type)
, m_presentation(other.m_presentation)
{}

Parameter::~Parameter()
{}

void Parameter::setDescription(const std::string &d)
{
    m_description = d;
}

void Parameter::setChoices(const std::vector<std::string> &c)
{
    checkChoice(c);
    m_choices = c;
}

int Parameter::module() const
{
    return m_module;
}

const std::string &Parameter::getName() const
{
    return m_name;
}

Parameter::Type Parameter::type() const
{
    return m_type;
}

void Parameter::setPresentation(Parameter::Presentation pres)
{
    m_presentation = pres;
}

Parameter::Presentation Parameter::presentation() const
{
    return m_presentation;
}

const std::vector<std::string> &Parameter::choices() const
{
    return m_choices;
}

const std::string &Parameter::description() const
{
    return m_description;
}

void Parameter::setGroup(const std::string &group)
{
    m_group = group;
}

const std::string &Parameter::group() const
{
    return m_group;
}

void Parameter::setGroupExpanded(bool expanded)
{
    m_groupExpanded = expanded;
}

bool Parameter::isGroupExpanded() const
{
    return m_groupExpanded;
}

void Parameter::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool Parameter::isReadOnly() const
{
    return m_readOnly;
}

void Parameter::setImmediate(bool immed)
{
    m_immediate = immed;
}

bool Parameter::isImmediate() const
{
    return m_immediate;
}

namespace {

using namespace boost;

struct instantiator {
    template<typename P>
    P operator()(P)
    {
        return P();
    }
};
} // namespace

void instantiate_parameters()
{
    mpl::for_each<Parameters>(instantiator());
}

std::shared_ptr<Parameter> getParameter(int moduleId, const std::string &paramName, Parameter::Type type)
{
    std::shared_ptr<Parameter> p;
    switch (type) {
    case Parameter::Integer:
        p.reset(new IntParameter(moduleId, paramName));
        break;
    case Parameter::Float:
        p.reset(new FloatParameter(moduleId, paramName));
        break;
    case Parameter::Vector:
        p.reset(new VectorParameter(moduleId, paramName));
        break;
    case Parameter::IntVector:
        p.reset(new IntVectorParameter(moduleId, paramName));
        break;
    case Parameter::String:
        p.reset(new StringParameter(moduleId, paramName));
        break;
    case Parameter::StringVector:
        p.reset(new StringVectorParameter(moduleId, paramName));
        break;
    case Parameter::Invalid:
    case Parameter::Unknown:
        break;
    }

    return p;
}

} // namespace vistle
