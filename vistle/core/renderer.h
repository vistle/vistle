#ifndef RENDERER_H
#define RENDERER_H

#include "module.h"

namespace vistle {
class Renderer: public Module {

 public:
   Renderer(const std::string &name,
            const int rank, const int size, const int moduleID);
   virtual ~Renderer();

   bool dispatch();

 private:
   virtual void render() = 0;
};

} // namespace vistle

#endif
