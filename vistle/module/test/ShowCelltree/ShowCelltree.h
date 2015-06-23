#ifndef SHOWCELLTREE_H
#define SHOWCELLTREE_H

#include <module/module.h>

class ShowCelltree: public vistle::Module {

 public:
   ShowCelltree(const std::string &shmname, const std::string &name, int moduleID);
   ~ShowCelltree();

 private:
   vistle::IntParameter *m_maxDepth;
   virtual bool compute();
};

#endif
