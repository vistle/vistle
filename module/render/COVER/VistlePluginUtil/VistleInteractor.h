#ifndef VISTLE_COVER_VISTLEPLUGINUTIL_VISTLEINTERACTOR_H
#define VISTLE_COVER_VISTLEPLUGINUTIL_VISTLEINTERACTOR_H

#include <cover/coInteractor.h>
#include <vistle/core/message.h>
#include <vistle/core/messages.h>
#include <vistle/core/messagesender.h>

#include "export.h"
#include "VistleRenderObject.h"

namespace vistle {
class Module;
}

class V_PLUGINUTILEXPORT VistleInteractor: public opencover::coInteractor {
public:
    VistleInteractor(const vistle::MessageSender *sender, const std::string &moduleName, int moduleId);
    ~VistleInteractor();
    void setPluginName(const std::string &plugin);

    void addParam(const std::string &name, const vistle::message::AddParameter &msg);
    void removeParam(const std::string &name, const vistle::message::RemoveParameter &msg);
    void applyParam(const std::string &name, const vistle::message::SetParameter &msg);
    bool hasParams() const;

    /// returns true, if Interactor comes from same Module as interactor i;
    bool isSameModule(coInteractor *i) const override;

    /// returns true, if Interactor is exactly the same as interactor i;
    bool isSame(coInteractor *i) const override;

    /// execute the Module
    void executeModule() override;

    /// copy the Module to same host
    void copyModule() override;

    /// copy the Module to same host and execute the copied one
    void copyModuleExec() override;

    /// delete the Module
    void deleteModule() override;

    // --- all getParameter Functions
    //       - return -1 on fail (illegal type requested), 0 if ok
    //       - only work for coFeedback created parameter messages
    //       - do not change the value fields if type incorrect

    int getBooleanParam(const std::string &paraName, int &value) const override;
    int getIntScalarParam(const std::string &paraName, int &value) const override;
    int getFloatScalarParam(const std::string &paraName, float &value) const override;
    int getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const override;
    int getFloatSliderParam(const std::string &paraName, float &min, float &max, float &val) const override;
    int getIntVectorParam(const std::string &paraName, int &numElem, int *&val) const override;
    int getFloatVectorParam(const std::string &paraName, int &numElem, float *&val) const override;
    int getStringParam(const std::string &paraName, const char *&val) const override;
    int getChoiceParam(const std::string &paraName, int &num, char **&labels, int &active) const override;
    int getFileBrowserParam(const std::string &paraName, char *&val) const override;


    // --- set-Functions:

    /// set Boolean Parameter
    void setBooleanParam(const char *name, int val) override;

    /// set float scalar parameter
    void setScalarParam(const char *name, float val) override;

    /// set int scalar parameter
    void setScalarParam(const char *name, int val) override;

    /// set float slider parameter
    void setSliderParam(const char *name, float min, float max, float value) override;

    /// set int slider parameter
    void setSliderParam(const char *name, int min, int max, int value) override;

    /// set float Vector Param
    void setVectorParam(const char *name, int numElem, float *field) override;
    void setVectorParam(const char *name, float u, float v, float w) override;

    /// set int vector parameter
    void setVectorParam(const char *name, int numElem, int *field) override;
    void setVectorParam(const char *name, int u, int v, int w) override;

    /// set string parameter
    void setStringParam(const char *name, const char *val) override;

    /// set choice parameter, pos starts with 0
    void setChoiceParam(const char *name, int pos) override;
    void setChoiceParam(const char *name, int num, const char *const *list, int pos) override;

    /// set browser parameter
    void setFileBrowserParam(const char *name, const char *val) override;

    // name of the covise data object which has feedback attributes
    const char *getObjName() override;

    // the covise data object which has feedback attributes
    ModuleRenderObject *getObject() override;

    // get the name of the module which created the data object
    const char *getPluginName() override;

    // get the name of the module which created the data object
    const char *getModuleName() override;

    // get the instance number of the module which created the data object
    int getModuleInstance() override;

    // get the hostname of the module which created the data object
    const char *getModuleHost() override;

    // -- The following functions only works for coFeedback attributes
    /// Get the number of Parameters
    int getNumParam() const override;

    /// Get the number of User Strings
    int getNumUser() const override;

    // get a User-supplied string
    const char *getString(unsigned int i) const override;

private:
    const vistle::MessageSender *m_sender;
    std::string m_moduleName;
    std::string m_pluginName;
    std::string m_hubName;
    int m_moduleId;
    std::shared_ptr<ModuleRenderObject> m_object;

    typedef std::map<std::string, std::shared_ptr<vistle::Parameter>> ParameterMap;
    ParameterMap m_parameterMap;

    std::shared_ptr<vistle::Parameter> findParam(const std::string &name) const;
    void sendMessage(const vistle::message::Message &msg) const;
    void sendParamMessage(const std::shared_ptr<vistle::Parameter> param) const;

    // clean up arrays allocated for getVectorParam
    mutable std::vector<int *> intArrays;
    mutable std::vector<float *> floatArrays;
};

#endif
