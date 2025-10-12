#ifndef VISTLE_ATTACHSHADER_ATTACHSHADER_H
#define VISTLE_ATTACHSHADER_ATTACHSHADER_H

#include <vistle/module/module.h>

class AttachShader: public vistle::Module {
public:
    AttachShader(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;
    bool changeParameter(const vistle::Parameter *param) override;

    vistle::StringParameter *m_shader, *m_shaderParams;

    vistle::ResultCache<vistle::Object::ptr> m_cache;
};

#endif
