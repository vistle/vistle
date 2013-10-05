#include "VistleInteractor.h"
#include "VistleRenderObject.h"
#include <core/parameter.h>
#include <module/module.h>

using namespace vistle;

VistleInteractor::VistleInteractor(const Module *owner, const std::string &moduleName, int moduleId)
: m_owner(owner)
, m_moduleName(moduleName)
, m_moduleId(moduleId)
, m_object(new ModuleRenderObject(m_moduleName, m_moduleId))
{
}

VistleInteractor::~VistleInteractor()
{
   delete m_object;

   for (ParameterMap::iterator it = m_parameterMap.begin();
         it != m_parameterMap.end();
         ++it) {
      delete it->second;
   }
   m_parameterMap.clear();
}

vistle::Parameter *VistleInteractor::findParam(const std::string &name) const
{
   ParameterMap::const_iterator it = m_parameterMap.find(name);
   if (it == m_parameterMap.end())
      return NULL;

   return it->second;
}

void VistleInteractor::addParam(const std::string &name, const vistle::message::AddParameter &msg)
{
   if (findParam(name))
      return;
   m_parameterMap[name] = msg.getParameter();
}

void VistleInteractor::applyParam(const std::string &name, const vistle::message::SetParameter &msg)
{
   Parameter *param = findParam(name);
   if (!param)
      return;
   msg.apply(param);
}

void VistleInteractor::removedObject()
{
}

/// returns true, if Interactor comes from same Module as intteractor i;
bool VistleInteractor::isSameModule(coInteractor* i) const
{
   VistleInteractor *inter = dynamic_cast<VistleInteractor *>(i);
   if (!i)
      return false;

   return m_moduleId == inter->m_moduleId;
}

/// returns true, if Interactor is exactly the same as interactor i;
bool VistleInteractor::isSame(coInteractor* i) const
{

   return isSameModule(i);
}

/// execute the Module
void VistleInteractor::executeModule()
{
   message::Compute m(m_moduleId, -1); // Communicator will update execution count
   sendMessage(m);
}

/// copy the Module to same host
void VistleInteractor::copyModule()
{
}

/// copy the Module to same host and execute the copied one
void VistleInteractor::copyModuleExec()
{
}

/// delete the Module
void VistleInteractor::deleteModule()
{
}

// --- all getParameter Functions
//       - return -1 on fail (illegal type requested), 0 if ok
//       - only work for coFeedback created parameter messages
//       - do not change the value fields if type incorrect

int VistleInteractor::getBooleanParam(const std::string &paraName, int &value) const
{
   return -1;
}

int VistleInteractor::getIntScalarParam(const std::string &paraName, int &value) const
{
   Parameter *param = findParam(paraName);
   if (!param)
      return -1;
   IntParameter *iparam = dynamic_cast<IntParameter *>(param);
   if (!iparam)
      return -1;

   value = iparam->getValue();

   return 0;
}

int VistleInteractor::getFloatScalarParam(const std::string &paraName, float &value) const
{
   Parameter *param = findParam(paraName);
   if (!param)
      return -1;
   FloatParameter *fparam = dynamic_cast<FloatParameter *>(param);
   if (!fparam)
      return -1;

   value = fparam->getValue();

   return 0;
}

int VistleInteractor::getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const
{
   return -1;
}

int VistleInteractor::getFloatSliderParam(const std::string &paraName, float &min, float &max, float &val) const
{
   Parameter *param = findParam(paraName);
   if (!param)
      return -1;
   FloatParameter *fparam = dynamic_cast<FloatParameter *>(param);
   if (!fparam)
      return -1;

   val = fparam->getValue();
   min = val - 3.;
   max = val + 3.;

   return 0;
}

int VistleInteractor::getIntVectorParam(const std::string &paraName, int &numElem, int *&val) const
{
   return -1;
}

int VistleInteractor::getFloatVectorParam(const std::string &paraName, int &numElem, float *&val) const
{
   Parameter *param = findParam(paraName);
   if (!param)
      return -1;
   VectorParameter *vparam = dynamic_cast<VectorParameter *>(param);
   if (!vparam)
      return -1;

   const ParamVector &v = vparam->getValue();
   numElem = v.dim;
   val = new float[numElem]; //FIXME: memory leak
   for (int i=0; i<numElem; ++i) {
      val[i] = v[i];
   }
   return 0;
}

int VistleInteractor::VistleInteractor::getStringParam(const std::string &paraName, const char *&val) const
{
   Parameter *param = findParam(paraName);
   if (!param)
      return -1;

   StringParameter *sparam = dynamic_cast<StringParameter *>(param);
   if (!sparam)
      return -1;

   val = sparam->getValue().c_str();

   return 0;
}

int VistleInteractor::getChoiceParam(const std::string &paraName, int &num, char **&labels, int &active) const
{
   return -1;
}

int VistleInteractor::getFileBrowserParam(const std::string &paraName, char *&val) const
{
   return -1;
}


// --- set-Functions:

void VistleInteractor::sendMessage(const message::Message &msg) const
{
   m_owner->sendMessage(msg);
}

void VistleInteractor::sendParamMessage(const Parameter *param) const
{
   message::SetParameter m(m_moduleId, param->getName(), param);
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
   Parameter *param = findParam(name);
   FloatParameter *fparam = dynamic_cast<FloatParameter *>(param);
   if (!fparam)
      return;
   fparam->setValue(val);
   sendParamMessage(fparam);
}

/// set int scalar parameter
void VistleInteractor::setScalarParam(const char *name, int val)
{
   Parameter *param = findParam(name);
   IntParameter *iparam = dynamic_cast<IntParameter *>(param);
   if (!iparam)
      return;
   iparam->setValue(val);
   sendParamMessage(iparam);
}

/// set float slider parameter
void VistleInteractor::setSliderParam(const char *name,float min,float max, float value)
{
   Parameter *param = findParam(name);
   FloatParameter *fparam = dynamic_cast<FloatParameter *>(param);
   if (!fparam)
      return;
   fparam->setValue(value);
   sendParamMessage(fparam);
}

/// set int slider parameter
void VistleInteractor::setSliderParam(const char *name, int min, int max, int value)
{
}

/// set float Vector Param
void VistleInteractor::setVectorParam(const char *name, int numElem, float *field)
{
   Parameter *param = findParam(name);
   VectorParameter *vparam = dynamic_cast<VectorParameter *>(param);
   if (!vparam)
      return;
   assert(vparam->getValue().dim == numElem);
   std::vector<ParamVector::value_type> v;
   for (int i=0; i<numElem; ++i)
      v.push_back(field[i]);
   vparam->setValue(ParamVector(numElem, &v[0]));
   sendParamMessage(vparam);
}

void VistleInteractor::setVectorParam(const char *name, float u, float v, float w)
{
   float vec[] = { u, v, w };
   setVectorParam(name, sizeof(vec)/sizeof(vec[0]), vec);
}

/// set int vector parameter
void VistleInteractor::setVectorParam(const char *name, int numElem, int *field)
{
}
void VistleInteractor::setVectorParam(const char *name, int u, int v, int w)
{
}

/// set string parameter
void VistleInteractor::setStringParam(const char *name, const char *val)
{
}

/// set choice parameter, pos starts with 1
void VistleInteractor::setChoiceParam(const char *name, int num , const char * const *list, int pos)
{
}

/// set browser parameter
void VistleInteractor::setFileBrowserParam(const char *name, const char *val)
{
}

// name of the covise data object which has feedback attributes
const char *VistleInteractor::getObjName()
{
   return m_object->getName();
}

// the covise data object which has feedback attributes
opencover::RenderObject *VistleInteractor::getObject()
{
   return m_object;
}

// get the name of the module which created the data object
const char *VistleInteractor::getPluginName()
{
   return m_moduleName.c_str();
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
   static const char *localhost = "localhost";
   return localhost;
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
const char *VistleInteractor::VistleInteractor::getString(unsigned int i) const
{
   return NULL;
}

