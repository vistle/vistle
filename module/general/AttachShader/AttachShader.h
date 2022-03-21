#ifndef ATTACHSHADER_H
#define ATTACHSHADER_H

#include <vistle/module/module.h>

class AttachShader: public vistle::Module {
public:
    AttachShader(const std::string &name, int moduleID, mpi::communicator comm);
    ~AttachShader();

private:
    virtual bool compute();

    vistle::StringParameter *m_shader, *m_shaderParams;

    vistle::ResultCache<vistle::Object::ptr> m_cache;
};

#endif
