#ifndef VISTLE_PARAMETER_MANAGER_H
#define VISTLE_PARAMETER_MANAGER_H

#include "export.h"

#include <vistle/core/messages.h>
#include <vistle/core/parameter.h>

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace vistle {

class V_COREEXPORT ParameterManager {
public:
    ParameterManager(const std::string &name, int id);
    virtual ~ParameterManager();

    //! set group for all subsequently added parameters, reset with empty group
    void setCurrentParameterGroup(const std::string &group = std::string(), bool defaultExpanded = true);
    const std::string &currentParameterGroup() const;

    virtual Parameter *addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> parameter);
    bool updateParameter(const std::string &name, const Parameter *parameter, const message::SetParameter *inResponseTo,
                         Parameter::RangeType rt = Parameter::Value);

    template<class T>
    Parameter *addParameter(const std::string &name, const std::string &description, const T &value,
                            Parameter::Presentation presentation = Parameter::Generic);
    template<class T>
    bool setParameter(const std::string &name, const T &value, const message::SetParameter *inResponseTo = nullptr);
    template<class T>
    bool setParameter(ParameterBase<T> *param, const T &value, const message::SetParameter *inResponseTo = nullptr);
    template<class T>
    bool setParameterMinimum(ParameterBase<T> *param, const T &minimum);
    template<class T>
    bool setParameterMaximum(ParameterBase<T> *param, const T &maximum);
    template<class T>
    bool setParameterRange(const std::string &name, const T &minimum, const T &maximum);
    template<class T>
    bool setParameterRange(ParameterBase<T> *param, const T &minimum, const T &maximum);
    template<class T>
    bool getParameter(const std::string &name, T &value) const;
    void setParameterChoices(const std::string &name, const std::vector<std::string> &choices);
    void setParameterChoices(Parameter *param, const std::vector<std::string> &choices);
    void setParameterFilters(const std::string &name, const std::string &filters);
    void setParameterFilters(StringParameter *param, const std::string &filters);
    bool setParameterImmediate(Parameter *param, bool immed);
    bool setParameterImmediate(const std::string &name, bool immed);

    StringParameter *addStringParameter(const std::string &name, const std::string &description,
                                        const std::string &value, Parameter::Presentation p = Parameter::Generic);
    bool setStringParameter(const std::string &name, const std::string &value,
                            const message::SetParameter *inResponseTo = NULL);
    std::string getStringParameter(const std::string &name) const;

    FloatParameter *addFloatParameter(const std::string &name, const std::string &description, const Float value);
    bool setFloatParameter(const std::string &name, const Float value,
                           const message::SetParameter *inResponseTo = NULL);
    Float getFloatParameter(const std::string &name) const;

    IntParameter *addIntParameter(const std::string &name, const std::string &description, const Integer value,
                                  Parameter::Presentation p = Parameter::Generic);
    bool setIntParameter(const std::string &name, const Integer value,
                         const message::SetParameter *inResponseTo = NULL);
    Integer getIntParameter(const std::string &name) const;

    VectorParameter *addVectorParameter(const std::string &name, const std::string &description,
                                        const ParamVector &value);
    bool setVectorParameter(const std::string &name, const ParamVector &value,
                            const message::SetParameter *inResponseTo = NULL);
    ParamVector getVectorParameter(const std::string &name) const;

    IntVectorParameter *addIntVectorParameter(const std::string &name, const std::string &description,
                                              const IntParamVector &value);
    bool setIntVectorParameter(const std::string &name, const IntParamVector &value,
                               const message::SetParameter *inResponseTo = NULL);
    IntParamVector getIntVectorParameter(const std::string &name) const;

    bool removeParameter(const std::string &name);
    virtual bool removeParameter(Parameter *param);

    std::shared_ptr<Parameter> findParameter(const std::string &name) const;

    void init();
    void quit();
    bool handleMessage(const message::SetParameter &message);
    virtual void sendParameterMessage(const message::Message &message, const buffer *payload = nullptr) const = 0;
    template<class Payload>
    void sendParameterMessageWithPayload(message::Message &message, Payload &payload);
    virtual bool changeParameters(std::set<const Parameter *> params); //< notify that some parameters have been changed
    virtual bool changeParameter(const Parameter *p); //< notify that a parameter has been changed
    void setId(int id);
    int id() const;
    void setName(const std::string &name);

    void applyDelayedChanges();

private:
    bool parameterChangedWrapper(const Parameter *p); //< wrapper to prevent recursive calls to parameterChanged

    int m_id = message::Id::Invalid;
    std::string m_name = std::string("ParameterManager");
    std::string m_currentParameterGroup;
    bool m_currentParameterGroupExpanded = true;
    std::map<std::string, std::shared_ptr<Parameter>> parameters;
    bool m_inParameterChanged = false;
    std::vector<const Parameter *> m_delayedChanges;
};

} // namespace vistle

#include "parametermanager_impl.h"
#endif
