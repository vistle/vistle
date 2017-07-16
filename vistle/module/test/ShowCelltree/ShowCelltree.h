#ifndef SHOWCELLTREE_H
#define SHOWCELLTREE_H

#include <module/module.h>

class ShowCelltree: public vistle::Module {

 public:
   ShowCelltree(const std::string &name, int moduleID, mpi::communicator comm);
   ~ShowCelltree();

 private:
   vistle::IntParameter *m_maxDepth, *m_showLeft, *m_showRight, *m_showBox;
   virtual bool compute();
};

#endif
