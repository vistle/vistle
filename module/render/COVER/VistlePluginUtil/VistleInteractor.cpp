#include "VistleInteractor.h"
#include "VistleRenderObject.h"
#include <cover/coVRAnimationManager.h>
#include <vistle/core/parameter.h>
#include <vistle/module/module.h>
#include <vistle/core/statetracker.h>

using namespace vistle;

#define CERR std::cerr << "VistleInteractor(" << m_moduleName << "_" << m_moduleId << "): "

VistleInteractor::VistleInteractor(const MessageSender *sender, const std::string &moduleName, int moduleId)
: m_sender(sender)
, m_moduleName(moduleName)
, m_pluginName(moduleName)
, m_moduleId(moduleId)
, m_object(new ModuleRenderObject(m_moduleName, m_moduleId))
{
    m_hubName = "hub for " + std::to_string(moduleId);
}

VistleInteractor::~VistleInteractor()
{
    m_stringParams.clear();
    m_intVecParams.clear();
    m_floatVecParams.clear();
    m_parameterMap.clear();
}

const char *VistleInteractor::getModuleDisplayName() const
{
    return m_moduleDisplayName.c_str();
}

void VistleInteractor::setDisplayName(const std::string &name)
{
    if (name.empty()) {
        m_moduleDisplayName.clear();
        return;
    }
    m_moduleDisplayName = name + "_" + std::to_string(m_moduleId);
}

void VistleInteractor::setPluginName(const std::string &plugin)
{
    m_pluginName = plugin;
}

std::shared_ptr<vistle::Parameter> VistleInteractor::findParam(const std::string &name) const
{
    ParameterMap::const_iterator it = m_parameterMap.find(name);
    if (it == m_parameterMap.end())
        return std::shared_ptr<vistle::Parameter>();

    return it->second;
}

void VistleInteractor::addParam(const std::string &name, const vistle::message::AddParameter &msg)
{
    if (findParam(name))
        return;
    m_parameterMap[name] = msg.getParameter();
}

void VistleInteractor::removeParam(const std::string &name, const vistle::message::RemoveParameter &msg)
{
    auto it = m_parameterMap.find(name);
    if (it != m_parameterMap.end()) {
        m_parameterMap.erase(it);
    }
}

bool VistleInteractor::hasParams() const
{
    return !m_parameterMap.empty();
}

void VistleInteractor::applyParam(const std::string &name, const vistle::message::SetParameter &msg)
{
    auto param = findParam(name);
    if (!param)
        return;
    msg.apply(param);
}

/// returns true, if Interactor comes from same Module as intteractor i;
bool VistleInteractor::isSameModule(coInteractor *i) const
{
    VistleInteractor *inter = dynamic_cast<VistleInteractor *>(i);
    if (!i)
        return false;

    return m_moduleId == inter->m_moduleId;
}

/// returns true, if Interactor is exactly the same as interactor i;
bool VistleInteractor::isSame(coInteractor *i) const
{
    return isSameModule(i);
}

/// execute the Module
void VistleInteractor::executeModule()
{
    auto &anim = *opencover::coVRAnimationManager::instance();
    double dt = 0.;
    if (anim.animationRunning() && std::abs(anim.getCurrentSpeed()) > 0.) {
        dt = 1. / anim.getCurrentSpeed();
    }
    double t = anim.getAnimationFrame();

    message::CancelExecute cancel(m_moduleId);
    cancel.setDestId(message::Id::MasterHub);
    sendMessage(cancel);

    message::Execute exec(m_moduleId, t, dt);
    exec.setDestId(message::Id::MasterHub);
    sendMessage(exec);
}

/// copy the Module to same host
void VistleInteractor::copyModule()
{}

/// copy the Module to same host and execute the copied one
void VistleInteractor::copyModuleExec()
{}

/// delete the Module
void VistleInteractor::deleteModule()
{}

// --- all getParameter Functions
//       - return -1 on fail (illegal type requested), 0 if ok
//       - only work for coFeedback created parameter messages
//       - do not change the value fields if type incorrect

int VistleInteractor::getBooleanParam(const std::string &paraName, int &value) const
{
    return getIntScalarParam(paraName, value);
}

int VistleInteractor::getIntScalarParam(const std::string &paraName, int &value) const
{
    auto param = findParam(paraName);
    if (!param) {
        value = 0;
        return -1;
    }
    auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
    if (!iparam) {
        CERR << "getIntScalarParam: " << paraName << " is not an IntParameter" << std::endl;
        value = 0;
        return -1;
    }

    value = iparam->getValue();

    return 0;
}

int VistleInteractor::getFloatScalarParam(const std::string &paraName, float &value) const
{
    auto param = findParam(paraName);
    if (!param) {
        value = 0.;
        return -1;
    }
    auto fparam = std::dynamic_pointer_cast<FloatParameter>(param);
    if (!fparam) {
        CERR << "getFloatScalarParam: " << paraName << " is not a FloatParameter" << std::endl;
        value = 0.;
        return -1;
    }

    value = fparam->getValue();

    return 0;
}

int VistleInteractor::getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const
{
    auto param = findParam(paraName);
    if (!param) {
        min = 0;
        val = 0;
        max = 100;
        return -1;
    }
    auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
    if (!iparam) {
        CERR << "getIntSliderParam: " << paraName << " is not an IntParameter" << std::endl;
        min = 0;
        val = 0;
        max = 100;
        return -1;
    }

    val = iparam->getValue();

    if (iparam->minimum() <= std::numeric_limits<int>::min() || iparam->maximum() >= std::numeric_limits<int>::max()) {
        min = val - 50;
        max = val + 50;
    } else {
        min = iparam->minimum();
        max = iparam->maximum();
    }

    return 0;
}

int VistleInteractor::getFloatSliderParam(const std::string &paraName, float &min, float &max, float &val) const
{
    auto param = findParam(paraName);
    if (!param) {
        min = -1.;
        val = 0.;
        max = 1.;
        return -1;
    }
    auto fparam = std::dynamic_pointer_cast<FloatParameter>(param);
    if (!fparam) {
        CERR << "getFloatSliderParam: " << paraName << " is not a FloatParameter" << std::endl;
        min = -1.;
        val = 0.;
        max = 1.;
        return -1;
    }

    val = fparam->getValue();

    if (fparam->minimum() <= -std::numeric_limits<float>::max() ||
        fparam->maximum() >= std::numeric_limits<float>::max()) {
        min = val - 5.;
        max = val + 5.;
    } else {
        min = fparam->minimum();
        max = fparam->maximum();
    }

    return 0;
}

int VistleInteractor::getIntVectorParam(const std::string &paraName, int &numElem, int *&val) const
{
    auto param = findParam(paraName);
    if (!param) {
        return -1;
    }
    auto vparam = std::dynamic_pointer_cast<IntVectorParameter>(param);
    if (!vparam) {
        CERR << "getIntVectorParam: " << paraName << " is not an IntVectorParameter" << std::endl;
        return -1;
    }

    const IntParamVector &v = vparam->getValue();
    numElem = v.dim;
    auto &result = m_intVecParams[paraName];
    result.resize(numElem);
    val = result.data();
    for (int i = 0; i < numElem; ++i) {
        val[i] = v[i];
    }
    return 0;
}

int VistleInteractor::getFloatVectorParam(const std::string &paraName, int &numElem, float *&val) const
{
    auto param = findParam(paraName);
    if (!param) {
        return -1;
    }
    auto vparam = std::dynamic_pointer_cast<VectorParameter>(param);
    if (!vparam) {
        CERR << "getFloatVectorParam: " << paraName << " is not a VectorParameter" << std::endl;
        return -1;
    }

    const ParamVector &v = vparam->getValue();
    numElem = v.dim;
    auto &result = m_floatVecParams[paraName];
    result.resize(numElem);
    val = result.data();
    for (int i = 0; i < numElem; ++i) {
        val[i] = v[i];
    }
    return 0;
}

int VistleInteractor::getStringParam(const std::string &paraName, const char *&val) const
{
    auto param = findParam(paraName);
    if (!param) {
        return -1;
    }

    auto sparam = std::dynamic_pointer_cast<StringParameter>(param);
    if (!sparam) {
        CERR << "getStringParam: " << paraName << " is not a StringParameter" << std::endl;
        return -1;
    }

    auto &result = m_stringParams[paraName];
    result = sparam->getValue();
    val = result.c_str();

    return 0;
}

int VistleInteractor::getChoiceParam(const std::string &paraName, int &num, char **&labels, int &active) const
{
    static std::vector<std::string> s_choices;
    static std::vector<const char *> s_labels;

    auto param = findParam(paraName);
    if (!param) {
        return -1;
    }
    if (param->presentation() != Parameter::Choice) {
        CERR << "getChoiceParam: " << paraName << " has no Choice presentation" << std::endl;
        return -1;
    }

    if (auto sparam = std::dynamic_pointer_cast<StringParameter>(param)) {
        s_choices = sparam->choices();
        std::string val = sparam->getValue();
        size_t idx = 0;
        bool found = false;
        for (auto &c: s_choices) {
            if (c == val) {
                found = true;
                break;
            }
            ++idx;
        }
        if (found) {
            active = idx;
        } else {
            return -1;
        }
    } else if (auto iparam = std::dynamic_pointer_cast<IntParameter>(param)) {
        s_choices = iparam->choices();
        num = s_choices.size();
        active = iparam->getValue();
    } else {
        CERR << "getChoiceParam: " << paraName << " is neither string nor int" << std::endl;
        return -1;
    }

    num = s_choices.size();
    s_labels.clear();
    for (auto &c: s_choices) {
        s_labels.push_back(c.c_str());
    }
    labels = &labels[0];

    return 0;
}

int VistleInteractor::getFileBrowserParam(const std::string &paraName, char *&val) const
{
    return -1;
}


// --- set-Functions:

void VistleInteractor::sendMessage(const message::Message &msg) const
{
    m_sender->sendMessage(msg);
}

void VistleInteractor::sendParamMessage(const std::shared_ptr<Parameter> param) const
{
    message::SetParameter m(m_moduleId, param->getName(), param);
    m.setDestId(m_moduleId);
    if (param->isImmediate()) {
        m.setPriority(message::Message::ImmediateParameter);
    }
    sendMessage(m);
}

/// set Boolean Parameter
void VistleInteractor::setBooleanParam(const char *name, int val)
{
    if (val)
        setScalarParam(name, (int)1);
    else
        setScalarParam(name, (int)0);
}

/// set float scalar parameter
void VistleInteractor::setScalarParam(const char *name, float val)
{
    auto param = findParam(name);
    auto fparam = std::dynamic_pointer_cast<FloatParameter>(param);
    if (!fparam)
        return;
    fparam->setValue(val);
    sendParamMessage(fparam);
}

/// set int scalar parameter
void VistleInteractor::setScalarParam(const char *name, int val)
{
    auto param = findParam(name);
    auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
    if (!iparam)
        return;
    iparam->setValue(val);
    sendParamMessage(iparam);
}

/// set float slider parameter
void VistleInteractor::setSliderParam(const char *name, float min, float max, float value)
{
    auto param = findParam(name);
    auto fparam = std::dynamic_pointer_cast<FloatParameter>(param);
    if (!fparam)
        return;
    fparam->setValue(value);
    sendParamMessage(fparam);
}

/// set int slider parameter
void VistleInteractor::setSliderParam(const char *name, int min, int max, int value)
{
    auto param = findParam(name);
    auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
    if (!iparam)
        return;
    iparam->setValue(value);
    sendParamMessage(iparam);
}

/// set float Vector Param
void VistleInteractor::setVectorParam(const char *name, int numElem, float *field)
{
    auto param = findParam(name);
    auto vparam = std::dynamic_pointer_cast<VectorParameter>(param);
    if (!vparam)
        return;
    assert(vparam->getValue().dim == numElem);
    std::vector<ParamVector::value_type> v;
    for (int i = 0; i < numElem; ++i)
        v.push_back(field[i]);
    vparam->setValue(ParamVector(numElem, &v[0]));
    sendParamMessage(vparam);
}

void VistleInteractor::setVectorParam(const char *name, float u, float v, float w)
{
    float vec[] = {u, v, w};
    setVectorParam(name, sizeof(vec) / sizeof(vec[0]), vec);
}

/// set int vector parameter
void VistleInteractor::setVectorParam(const char *name, int numElem, int *field)
{
    auto param = findParam(name);
    auto vparam = std::dynamic_pointer_cast<IntVectorParameter>(param);
    if (!vparam)
        return;
    assert(vparam->getValue().dim == numElem);
    std::vector<IntParamVector::value_type> v;
    for (int i = 0; i < numElem; ++i)
        v.push_back(field[i]);
    vparam->setValue(IntParamVector(numElem, &v[0]));
    sendParamMessage(vparam);
}

void VistleInteractor::setVectorParam(const char *name, int u, int v, int w)
{
    int vec[] = {u, v, w};
    setVectorParam(name, sizeof(vec) / sizeof(vec[0]), vec);
}

/// set string parameter
void VistleInteractor::setStringParam(const char *name, const char *val)
{
    auto param = findParam(name);
    auto sparam = std::dynamic_pointer_cast<StringParameter>(param);
    if (!sparam)
        return;
    sparam->setValue(val);
    sendParamMessage(sparam);
}

/// set choice parameter, pos starts with 0
void VistleInteractor::setChoiceParam(const char *name, int pos)
{
    static std::vector<std::string> s_choices;
    static std::vector<const char *> s_labels;

    auto param = findParam(name);
    if (!param)
        return;
    if (param->presentation() != Parameter::Choice)
        return;

    if (auto sparam = std::dynamic_pointer_cast<StringParameter>(param)) {
        CERR << "cannot set string choice parameter based on position" << std::endl;
    } else if (auto iparam = std::dynamic_pointer_cast<IntParameter>(param)) {
        iparam->setValue(pos);
        sendParamMessage(iparam);
    } else {
        return;
    }
}

void VistleInteractor::setChoiceParam(const char *name, int num, const char *const *list, int pos)
{
    static std::vector<std::string> s_choices;
    static std::vector<const char *> s_labels;

    auto param = findParam(name);
    if (!param)
        return;
    if (param->presentation() != Parameter::Choice)
        return;

    std::vector<std::string> choices;
    for (int i = 0; i < num; ++i) {
        choices.emplace_back(list[i]);
    }

#if 0
   {
      message::SetParameterChoices sc(name, choices);
      sc.setDestId(param->module());
      sendMessage(sc);
   }
#endif

    if (auto sparam = std::dynamic_pointer_cast<StringParameter>(param)) {
        auto val = choices[pos];
        sparam->setValue(val);
        sendParamMessage(sparam);
    } else if (auto iparam = std::dynamic_pointer_cast<IntParameter>(param)) {
        iparam->setValue(pos);
        sendParamMessage(iparam);
    } else {
        return;
    }
}

/// set browser parameter
void VistleInteractor::setFileBrowserParam(const char *name, const char *val)
{}

// name of the covise data object which has feedback attributes
const char *VistleInteractor::getObjName()
{
    static const char empty[] = "";
    if (!m_object)
        return empty;
    return m_object->getName();
}

// the covise data object which has feedback attributes
ModuleRenderObject *VistleInteractor::getObject()
{
    return m_object.get();
}

// get the name of the module which created the data object
const char *VistleInteractor::getPluginName()
{
    return m_pluginName.c_str();
}

// get the name of the module which created the data object
const char *VistleInteractor::getModuleName()
{
    return m_moduleName.c_str();
}

// get the instance number of the module which created the data object
int VistleInteractor::getModuleInstance()
{
    return m_moduleId;
}

// get the hostname of the module which created the data object
const char *VistleInteractor::getModuleHost()
{
    return m_hubName.c_str();
}

// -- The following functions only works for coFeedback attributes
/// Get the number of Parameters
int VistleInteractor::getNumParam() const
{
    return m_parameterMap.size();
}

/// Get the number of User Strings
int VistleInteractor::getNumUser() const
{
    return 0;
}

// get a User-supplied string
const char *VistleInteractor::getString(unsigned int i) const
{
    return NULL;
}
