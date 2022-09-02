#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include <memory>
#include <boost/mpl/vector.hpp>

#include <vistle/util/enum.h>
#include "paramvector.h"
#include "export.h"

class VistleInteractor;

namespace gui {
class Parameters;
}

namespace std {
inline std::string to_string(const string &s)
{
    return s;
}
} // namespace std

namespace vistle {

typedef boost::mpl::vector<Integer, Float, ParamVector, std::string> Parameters;

class V_COREEXPORT Parameter {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Type,
                                        (Unknown) // keep first
                                        (Float)(Integer)(Vector)(IntVector)(String)(Invalid) // keep last
    )

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Presentation,
                                        (Generic) // default, keep first
                                        (Filename) // String
                                        (ExistingFilename) // String
                                        (Directory) // String
                                        (ExistingDirectory) // String
                                        (NewPathname) // String
                                        (Boolean) // Integer
                                        (Choice) // Integer (fixed choice) and String (dynamic choice)
                                        (Slider) // Integer, Float
                                        (Color) // Vector3
                                        (InvalidPresentation) // keep last
    )

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(RangeType,
                                        // keep in that order
                                        (Minimum) // also used for filebrowser filters
                                        (Value) // update parameter value
                                        (Maximum) // update allowed parameter maximum
                                        (Other) // update other information than values and range
    )

    Parameter(int moduleId, const std::string &name, Type = Invalid, Presentation = Generic);
    Parameter(const Parameter &other);
    virtual ~Parameter();

    virtual Parameter *clone() const = 0;

    void setPresentation(Presentation presentation);
    void setDescription(const std::string &description);
    void setChoices(const std::vector<std::string> &choices);
    void setReadOnly(bool readOnly);

    void setGroup(const std::string &group);
    const std::string &group() const;
    void setGroupExpanded(bool expanded);
    bool isGroupExpanded() const;

    virtual operator std::string() const = 0;
    virtual bool isDefault() const = 0;
    virtual bool checkChoice(const std::vector<std::string> &choices) const { return true; }
    int module() const;
    const std::string &getName() const;
    Type type() const;
    Presentation presentation() const;
    const std::string &description() const;
    const std::vector<std::string> &choices() const;
    bool isReadOnly() const;
    void setImmediate(bool immed);
    bool isImmediate() const;

protected:
    std::vector<std::string> m_choices;

private:
    int m_module;
    std::string m_name;
    std::string m_description;
    std::string m_group;
    enum Type m_type;
    enum Presentation m_presentation;
    bool m_groupExpanded = true;
    bool m_immediate = false;
    bool m_readOnly = false;
};

template<typename T>
struct ParameterType {};

template<typename T>
struct ParameterCheck {
    static bool check(const std::vector<std::string> &, const T &) { return true; }
};

namespace message {
class SetParameter;
}

template<typename T>
class ParameterBase: public Parameter {
    friend class ParameterManager;
    friend class Module;
    friend class Renderer;
    friend class message::SetParameter;
    friend class VistleConnection;
    friend class gui::Parameters;
    friend class ::VistleInteractor;

    virtual bool setValue(T value, bool init = false, bool delayed = false)
    {
        if (init)
            m_defaultValue = value;
        else if (!delayed && !checkValue(value))
            return false;
        this->m_value = value;
        return true;
    }
    virtual void setMinimum(const T &value) { m_minimum = value; }
    virtual void setMaximum(const T &value) { m_maximum = value; }

public:
    typedef T ValueType;

    ParameterBase(int moduleId, const std::string &name, T value = T())
    : Parameter(moduleId, name, ParameterType<T>::type)
    , m_value(value)
    , m_defaultValue(value)
    , m_minimum(ParameterType<T>::min())
    , m_maximum(ParameterType<T>::max())
    {}
    ParameterBase(const ParameterBase<T> &other)
    : Parameter(other)
    , m_value(other.m_value)
    , m_defaultValue(other.m_defaultValue)
    , m_minimum(other.m_minimum)
    , m_maximum(other.m_maximum)
    {}
    virtual ~ParameterBase() {}

    virtual ParameterBase<T> *clone() const { return new ParameterBase<T>(*this); }

    bool isDefault() const { return m_value == m_defaultValue; }
    const T getDefaultValue() const { return m_defaultValue; }
    const T getValue() const { return m_value; }
    const T getValue(RangeType rt) const
    {
        switch (rt) {
        case Minimum:
            return m_minimum;
        case Maximum:
            return m_maximum;
        case Value:
        case Other:
            break;
        }
        return m_value;
    }
    virtual bool checkValue(const T &value) const
    {
        if (ParameterType<T>::isNumber) {
            if (value < m_minimum)
                return false;
            if (value > m_maximum)
                return false;
        }
        if (presentation() != Choice)
            return true;
        return ParameterCheck<T>::check(m_choices, value);
    }
    virtual bool checkChoice(const std::vector<std::string> &ch) const
    {
        if (presentation() != Choice)
            return false;
        return ParameterCheck<T>::check(ch, m_value);
    }
    virtual T minimum() const { return m_minimum; }
    virtual T maximum() const { return m_maximum; }
    operator std::string() const { return std::to_string(m_value); }

private:
    T m_value;
    T m_defaultValue;
    T m_minimum, m_maximum;
};

template<>
struct V_COREEXPORT ParameterType<Integer> {
    typedef Integer T;
    static const Parameter::Type type = Parameter::Integer;
    static const bool isNumber = true;
    static T min() { return std::numeric_limits<T>::lowest(); }
    static T max() { return std::numeric_limits<T>::max(); }
};

template<>
struct V_COREEXPORT ParameterType<Float> {
    typedef Float T;
    static const Parameter::Type type = Parameter::Float;
    static const bool isNumber = true;
    static T min() { return std::numeric_limits<T>::lowest(); }
    static T max() { return std::numeric_limits<T>::max(); }
};

template<>
struct V_COREEXPORT ParameterType<ParamVector> {
    typedef ParamVector T;
    static const Parameter::Type type = Parameter::Vector;
    static const bool isNumber = true;
    static T min() { return T::min(); }
    static T max() { return T::max(); }
};

template<>
struct V_COREEXPORT ParameterType<IntParamVector> {
    typedef IntParamVector T;
    static const Parameter::Type type = Parameter::IntVector;
    static const bool isNumber = true;
    static T min() { return T::min(); }
    static T max() { return T::max(); }
};

template<>
struct V_COREEXPORT ParameterType<std::string> {
    typedef std::string T;
    static const Parameter::Type type = Parameter::String;
    static const bool isNumber = false;
    static T min() { return ""; }
    static T max() { return ""; }
};

template<>
struct ParameterCheck<Integer> {
    static bool check(const std::vector<std::string> &choices, const Integer &value)
    {
        if (choices.empty()) {
            // choices not yet initialized
            return true;
        }
        if (value < 0 || size_t(value) >= choices.size()) {
            std::cerr << "IntParameter: choice out of range" << std::endl;
            return false;
        }
        return true;
    }
};

template<>
struct ParameterCheck<std::string> {
    static bool check(const std::vector<std::string> &choices, const std::string &value)
    {
        if (choices.empty()) {
            // choices not yet initialized
            return true;
        }
        if (std::find(choices.begin(), choices.end(), value) == choices.end()) {
            if (!value.empty())
                std::cerr << "StringParameter: choice \"" << value << "\" not valid" << std::endl;
            return false;
        }
        return true;
    }
};

#define V_PARAM_TYPE(ValueType, Name) \
    extern template class V_COREEXPORT ParameterBase<ValueType>; \
    typedef ParameterBase<ValueType> Name;
V_PARAM_TYPE(ParamVector, VectorParameter)
V_PARAM_TYPE(IntParamVector, IntVectorParameter)
V_PARAM_TYPE(Float, FloatParameter)
V_PARAM_TYPE(Integer, IntParameter)
V_PARAM_TYPE(std::string, StringParameter)
#undef V_PARAM_TYPE

V_ENUM_OUTPUT_OP(Type, Parameter)
V_ENUM_OUTPUT_OP(Presentation, Parameter)
V_ENUM_OUTPUT_OP(RangeType, Parameter)

std::shared_ptr<Parameter> V_COREEXPORT getParameter(int moduleId, const std::string &paramName, Parameter::Type type,
                                                     Parameter::Presentation presentation);


} // namespace vistle
#endif
