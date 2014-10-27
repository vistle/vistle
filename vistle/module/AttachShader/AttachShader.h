#ifndef ATTACHSHADER_H
#define ATTACHSHADER_H

#include <module/module.h>

class AttachShader: public vistle::Module {

 public:
   AttachShader(const std::string &shmname, int rank, int size, int moduleID);
   ~AttachShader();

 private:
   virtual bool compute();

   vistle::StringParameter *m_shader, *m_shaderParams;
};

#endif
