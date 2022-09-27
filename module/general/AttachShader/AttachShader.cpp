#include <vistle/core/object.h>
#include <vistle/core/message.h>
#include <vistle/module/resultcache.h>
#include "AttachShader.h"

MODULE_MAIN(AttachShader)

using namespace vistle;

AttachShader::AttachShader(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    createInputPort("data_in", "input data or geometry");

    createOutputPort("data_out", "data or geometry with attribute requesting COVER to apply shader");

    m_shader = addStringParameter("shader", "name of shader to apply to geometry", "");
    m_shaderParams =
        addStringParameter("shader_params", "shader parameters (as \"key=value\" \"key=value1 value2\"", "");

    addResultCache(m_cache);
}

AttachShader::~AttachShader()
{}

bool AttachShader::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    Object::ptr nobj;
    if (auto *entry = m_cache.getOrLock(obj->getName(), nobj)) {
        nobj = obj->clone();
        nobj->addAttribute("shader", m_shader->getValue());
        if (!m_shaderParams->getValue().empty()) {
            nobj->addAttribute("shader_params", m_shaderParams->getValue());
        }
        updateMeta(nobj);
        m_cache.storeAndUnlock(entry, nobj);
    }
    addObject("data_out", nobj);

    return true;
}
