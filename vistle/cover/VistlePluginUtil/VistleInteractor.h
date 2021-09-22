#ifndef VISTLE_INTERACTOR_H
#define VISTLE_INTERACTOR_H

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
    VistleInteractor(const vistle::Module *owner, const std::string &moduleName, int moduleId);
    ~VistleInteractor();
    void setPluginName(const std::string &plugin);

    void addParam(const std::string &name, const vistle::message::AddParameter &msg);
    void removeParam(const std::string &name, const vistle::message::RemoveParameter &msg);
    void applyParam(const std::string &name, const vistle::message::SetParameter &msg);
    bool hasParams() const;

    virtual void removedObject();

    /// returns true, if Interactor comes from same Module as intteractor i;
    virtual bool isSameModule(coInteractor *i) const;

    /// returns true, if Interactor is exactly the same as interactor i;
    virtual bool isSame(coInteractor *i) const;

    /// execute the Module
    virtual void executeModule();

    /// copy the Module to same host
    virtual void copyModule();

    /// copy the Module to same host and execute the copied one
    virtual void copyModuleExec();

    /// delete the Module
    virtual void deleteModule();

    // --- all getParameter Functions
    //       - return -1 on fail (illegal type requested), 0 if ok
    //       - only work for coFeedback created parameter messages
    //       - do not change the value fields if type incorrect

    virtual int getBooleanParam(const std::string &paraName, int &value) const;
    virtual int getIntScalarParam(const std::string &paraName, int &value) const;
    virtual int getFloatScalarParam(const std::string &paraName, float &value) const;
    virtual int getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const;
    virtual int getFloatSliderParam(const std::string &paraName, float &min, float &max, float &val) const;
    virtual int getIntVectorParam(const std::string &paraName, int &numElem, int *&val) const;
    virtual int getFloatVectorParam(const std::string &paraName, int &numElem, float *&val) const;
    virtual int getStringParam(const std::string &paraName, const char *&val) const;
    virtual int getChoiceParam(const std::string &paraName, int &num, char **&labels, int &active) const;
    virtual int getFileBrowserParam(const std::string &paraName, char *&val) const;


    // --- set-Functions:

    /// set Boolean Parameter
    virtual void setBooleanParam(const char *name, int val);

    /// set float scalar parameter
    virtual void setScalarParam(const char *name, float val);

    /// set int scalar parameter
    virtual void setScalarParam(const char *name, int val);

    /// set float slider parameter
    virtual void setSliderParam(const char *name, float min, float max, float value);

    /// set int slider parameter
    virtual void setSliderParam(const char *name, int min, int max, int value);

    /// set float Vector Param
    virtual void setVectorParam(const char *name, int numElem, float *field);
    virtual void setVectorParam(const char *name, float u, float v, float w);

    /// set int vector parameter
    virtual void setVectorParam(const char *name, int numElem, int *field);
    virtual void setVectorParam(const char *name, int u, int v, int w);

    /// set string parameter
    virtual void setStringParam(const char *name, const char *val);

    /// set choice parameter, pos starts with 0
    virtual void setChoiceParam(const char *name, int pos);
    virtual void setChoiceParam(const char *name, int num, const char *const *list, int pos);

    /// set browser parameter
    virtual void setFileBrowserParam(const char *name, const char *val);

    // name of the covise data object which has feedback attributes
    virtual const char *getObjName();

    // the covise data object which has feedback attributes
    virtual ModuleRenderObject *getObject();

    // get the name of the module which created the data object
    virtual const char *getPluginName();

    // get the name of the module which created the data object
    virtual const char *getModuleName();

    // get the instance number of the module which created the data object
    virtual int getModuleInstance();

    // get the hostname of the module which created the data object
    virtual const char *getModuleHost();

    // -- The following functions only works for coFeedback attributes
    /// Get the number of Parameters
    virtual int getNumParam() const;

    /// Get the number of User Strings
    virtual int getNumUser() const;

    // get a User-supplied string
    virtual const char *getString(unsigned int i) const;

private:
    const vistle::MessageSender *m_owner;
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
