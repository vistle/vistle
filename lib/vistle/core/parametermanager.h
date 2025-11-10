#ifndef VISTLE_CORE_PARAMETERMANAGER_H
#define VISTLE_CORE_PARAMETERMANAGER_H

#include "export.h"

#include <vistle/core/messages.h>
#include <vistle/core/parameter.h>

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <deque>

namespace vistle {

namespace config {
class Access;
}

class V_COREEXPORT ParameterManager {
public:
    ParameterManager(const std::string &name, int id);
    virtual ~ParameterManager();

    static int parameterTargetModule(int id, const std::string &name);

    void setConfig(config::Access *config);

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
    bool setParameterReadOnly(Parameter *param, bool readOnly);
    bool setParameterReadOnly(const std::string &name, bool readOnly);
    bool setParameterImmediate(Parameter *param, bool immed);
    bool setParameterImmediate(const std::string &name, bool immed);

    StringParameter *addStringParameter(const std::string &name, const std::string &description,
                                        const std::string &value, Parameter::Presentation p = Parameter::Generic);
    bool setStringParameter(const std::string &name, const std::string &value,
                            const message::SetParameter *inResponseTo = NULL);
    std::string getStringParameter(const std::string &name) const;

    FloatParameter *addFloatParameter(const std::string &name, const std::string &description, const Float value,
                                      Parameter::Presentation p = Parameter::Generic);
    bool setFloatParameter(const std::string &name, const Float value,
                           const message::SetParameter *inResponseTo = NULL);
    Float getFloatParameter(const std::string &name) const;

    IntParameter *addIntParameter(const std::string &name, const std::string &description, const Integer value,
                                  Parameter::Presentation p = Parameter::Generic);
    bool setIntParameter(const std::string &name, const Integer value,
                         const message::SetParameter *inResponseTo = NULL);
    Integer getIntParameter(const std::string &name) const;

    VectorParameter *addVectorParameter(const std::string &name, const std::string &description, ParamVector value,
                                        Parameter::Presentation p = Parameter::Generic);
    bool setVectorParameter(const std::string &name, const ParamVector &value,
                            const message::SetParameter *inResponseTo = NULL);
    ParamVector getVectorParameter(const std::string &name) const;

    IntVectorParameter *addIntVectorParameter(const std::string &name, const std::string &description,
                                              IntParamVector value, Parameter::Presentation p = Parameter::Generic);
    bool setIntVectorParameter(const std::string &name, const IntParamVector &value,
                               const message::SetParameter *inResponseTo = NULL);
    IntParamVector getIntVectorParameter(const std::string &name) const;

    StringVectorParameter *addStringVectorParameter(const std::string &name, const std::string &description,
                                                    StringParamVector value,
                                                    Parameter::Presentation p = Parameter::Generic);
    bool setStringVectorParameter(const std::string &name, const StringParamVector &value,
                                  const message::SetParameter *inResponseTo = NULL);
    StringParamVector getStringVectorParameter(const std::string &name) const;

    virtual bool removeParameter(const std::string &name);

    std::shared_ptr<Parameter> findParameter(const std::string &name) const;

    void init();
    void quit();
    bool handleMessage(const message::SetParameter &message);
    bool handleMessage(const message::AddParameter &message);
    bool handleMessage(const message::RemoveParameter &message);
    virtual void sendParameterMessage(const message::Message &message, const buffer *payload = nullptr) const = 0;
    template<class Payload>
    void sendParameterMessageWithPayload(message::Message &message, Payload &payload);
    virtual bool
    changeParameters(std::map<std::string, const Parameter *> params); //< notify that some parameters have been changed
    virtual bool changeParameter(const Parameter *p); //< notify that a parameter has been changed
    void setId(int id);
    int id() const;
    void setName(const std::string &name);

    bool applyDelayedChanges();

private:
    bool resetParameterToDefault(const std::string &name, const message::SetParameter *inResponseTo = nullptr);
    template<class T>
    bool resetParameterToDefault(Parameter *parameter, const message::SetParameter *inResponseTo);
    bool parameterChangedWrapper(const Parameter *p); //< wrapper to prevent recursive calls to parameterChanged

    int m_id = message::Id::Invalid;
    std::string m_name = std::string("ParameterManager");
    std::string m_currentParameterGroup;
    bool m_currentParameterGroupExpanded = true;
    struct ParameterData {
        ParameterData() = default;
        ParameterData(std::shared_ptr<Parameter> &param): param(param) {}
        std::shared_ptr<Parameter> param;
        bool owner = true;
    };
    std::map<std::string, ParameterData> m_parameters;
    bool m_inParameterChanged = false;
    bool m_inApplyDelayedChanges = false;
    std::map<std::string, const Parameter *> m_delayedChanges;

    std::deque<message::SetParameter> m_queue;

    config::Access *m_config = nullptr;
};

#define PARAM_TYPE_TEMPLATE(spec, Type) \
    spec template V_COREEXPORT Parameter *ParameterManager::addParameter( \
        const std::string &name, const std::string &description, const Type &value, \
        Parameter::Presentation presentation); \
    spec template V_COREEXPORT bool ParameterManager::setParameter<Type>(const std::string &name, const Type &value, \
                                                                         const message::SetParameter *inResponseTo); \
    spec template V_COREEXPORT bool ParameterManager::setParameter<Type>(ParameterBase<Type> *, Type const &, \
                                                                         message::SetParameter const *); \
    spec template V_COREEXPORT bool ParameterManager::setParameterRange<Type>(const std::string &, Type const &, \
                                                                              Type const &); \
    spec template V_COREEXPORT bool ParameterManager::setParameterRange<Type>(ParameterBase<Type> *, Type const &, \
                                                                              Type const &); \
    spec template V_COREEXPORT bool ParameterManager::setParameterMinimum<Type>(ParameterBase<Type> *, Type const &); \
    spec template V_COREEXPORT bool ParameterManager::setParameterMaximum<Type>(ParameterBase<Type> *, Type const &);

PARAM_TYPE_TEMPLATE(extern, Integer)
PARAM_TYPE_TEMPLATE(extern, Float)
PARAM_TYPE_TEMPLATE(extern, std::string)
PARAM_TYPE_TEMPLATE(extern, ParameterVector<Integer>)
PARAM_TYPE_TEMPLATE(extern, ParameterVector<Float>)
PARAM_TYPE_TEMPLATE(extern, ParameterVector<std::string>)

} // namespace vistle

//#include "parametermanager_impl.h"
#endif
