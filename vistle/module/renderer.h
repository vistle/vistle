#ifndef RENDERER_H
#define RENDERER_H

#include "module.h"
#include "export.h"

namespace vistle {

class V_MODULEEXPORT Renderer: public Module {

 public:
   Renderer(const std::string &name, const std::string &shmname,
            const int rank, const int size, const int moduleID);
   virtual ~Renderer();

   bool dispatch();

   bool parameterChanged(Parameter *p);

 private:
   virtual void render() = 0;
   IntParameter *m_renderMode;
};

} // namespace vistle

#endif
