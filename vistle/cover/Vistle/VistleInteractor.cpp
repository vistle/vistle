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
   m_parameterMap.clear();
}

boost::shared_ptr<vistle::Parameter> VistleInteractor::findParam(const std::string &name) const
{
   ParameterMap::const_iterator it = m_parameterMap.find(name);
   if (it == m_parameterMap.end())
      return boost::shared_ptr<vistle::Parameter>();

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
   auto param = findParam(name);
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
   message::Execute m(message::Execute::ComputeExecute, m_moduleId); // Communicator will update execution count
   m.setDestId(message::Id::MasterHub);
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
   return getIntScalarParam(paraName, value);
}

int VistleInteractor::getIntScalarParam(const std::string &paraName, int &value) const
{
   auto param = findParam(paraName);
   if (!param)
      return -1;
   auto iparam = boost::dynamic_pointer_cast<IntParameter>(param);
   if (!iparam)
      return -1;

   value = iparam->getValue();

   return 0;
}

int VistleInteractor::getFloatScalarParam(const std::string &paraName, float &value) const
{
   auto param = findParam(paraName);
   if (!param)
      return -1;
   auto fparam = boost::dynamic_pointer_cast<FloatParameter>(param);
   if (!fparam)
      return -1;

   value = fparam->getValue();

   return 0;
}

int VistleInteractor::getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const
{
   auto param = findParam(paraName);
   if (!param)
      return -1;
   auto iparam = boost::dynamic_pointer_cast<IntParameter>(param);
   if (!iparam)
      return -1;

   val = iparam->getValue();

   if (iparam->minimum() <= std::numeric_limits<int>::min()
         || iparam->maximum() >= std::numeric_limits<int>::max()) {
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
   if (!param)
      return -1;
   auto fparam = boost::dynamic_pointer_cast<FloatParameter>(param);
   if (!fparam)
      return -1;

   val = fparam->getValue();

   if (fparam->minimum() <= -std::numeric_limits<float>::max()
         || fparam->maximum() >= std::numeric_limits<float>::max()) {
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
   if (!param)
      return -1;
   auto vparam = boost::dynamic_pointer_cast<IntVectorParameter>(param);
   if (!vparam)
      return -1;

   const IntParamVector &v = vparam->getValue();
   numElem = v.dim;
   val = new int[numElem]; //FIXME: memory leak
   for (int i=0; i<numElem; ++i) {
      val[i] = v[i];
   }
   return 0;
}

int VistleInteractor::getFloatVectorParam(const std::string &paraName, int &numElem, float *&val) const
{
   auto param = findParam(paraName);
   if (!param)
      return -1;
   auto vparam = boost::dynamic_pointer_cast<VectorParameter>(param);
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
   auto param = findParam(paraName);
   if (!param)
      return -1;

   auto sparam = boost::dynamic_pointer_cast<StringParameter>(param);
   if (!sparam)
      return -1;

   val = sparam->getValue().c_str();

   return 0;
}

int VistleInteractor::getChoiceParam(const std::string &paraName, int &num, char **&labels, int &active) const
{
   static std::vector<std::string> s_choices;
   static std::vector<const char *> s_labels;

   auto param = findParam(paraName);
   if (!param)
      return -1;
   if (param->presentation() != Parameter::Choice)
      return -1;

   if (auto sparam = boost::dynamic_pointer_cast<StringParameter>(param)) {
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
   } else if (auto iparam = boost::dynamic_pointer_cast<IntParameter>(param)) {
      s_choices = iparam->choices();
      num = s_choices.size();
      active = iparam->getValue();
   } else {
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
   m_owner->sendMessage(msg);
}

void VistleInteractor::sendParamMessage(const boost::shared_ptr<Parameter> param) const
{
   message::SetParameter m(m_moduleId, param->getName(), param);
   m.setDestId(m_moduleId);
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
   auto fparam = boost::dynamic_pointer_cast<FloatParameter>(param);
   if (!fparam)
      return;
   fparam->setValue(val);
   sendParamMessage(fparam);
}

/// set int scalar parameter
void VistleInteractor::setScalarParam(const char *name, int val)
{
   auto param = findParam(name);
   auto iparam = boost::dynamic_pointer_cast<IntParameter>(param);
   if (!iparam)
      return;
   iparam->setValue(val);
   sendParamMessage(iparam);
}

/// set float slider parameter
void VistleInteractor::setSliderParam(const char *name,float min,float max, float value)
{
   auto param = findParam(name);
   auto fparam = boost::dynamic_pointer_cast<FloatParameter>(param);
   if (!fparam)
      return;
   fparam->setValue(value);
   sendParamMessage(fparam);
}

/// set int slider parameter
void VistleInteractor::setSliderParam(const char *name, int min, int max, int value)
{
   auto param = findParam(name);
   auto iparam = boost::dynamic_pointer_cast<IntParameter>(param);
   if (!iparam)
      return;
   iparam->setValue(value);
   sendParamMessage(iparam);
}

/// set float Vector Param
void VistleInteractor::setVectorParam(const char *name, int numElem, float *field)
{
   auto param = findParam(name);
   auto vparam = boost::dynamic_pointer_cast<VectorParameter>(param);
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

