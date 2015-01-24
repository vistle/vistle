#include <core/object.h>
#include <core/message.h>
#include "AttachShader.h"

MODULE_MAIN(AttachShader)

using namespace vistle;

AttachShader::AttachShader(const std::string &shmname, const std::string &name, int moduleID)
    : Module("attach GLSL shader in COVER", shmname, name, moduleID) {

    setDefaultCacheMode(ObjectCache::CacheAll);

    createInputPort("data_in");

    createOutputPort("data_out");

    m_shader = addStringParameter("shader", "name of shader to apply to geometry", "");
    m_shaderParams = addStringParameter("shader_params", "shader parameters (as \"key=value\" \"key=value1 value2\"", "");
}

AttachShader::~AttachShader() {

}

bool AttachShader::compute() {

   Object::const_ptr obj = expect<Object>("data_in");
   if (!obj)
      return false;

   if (obj->isEmpty() || !m_shader->getValue().empty()) {
      passThroughObject("data_out", obj);
   } else {
      Object::ptr nobj = obj->clone();
      nobj->addAttribute("shader", m_shader->getValue());
      if (!m_shaderParams->getValue().empty()) {
         nobj->addAttribute("shader_params", m_shaderParams->getValue());
      }
      addObject("data_out", nobj);
   }

   return true;
}
