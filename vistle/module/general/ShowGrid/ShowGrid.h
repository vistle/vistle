#ifndef SHOWUSG_H
#define SHOWUSG_H

#include <module/module.h>

class ShowUSG: public vistle::Module {

 public:
   ShowUSG(const std::string &name, int moduleID, mpi::communicator comm);
   ~ShowUSG();

 private:
   virtual bool compute();

   vistle::IntParameter *m_CellNrMin;
   vistle::IntParameter *m_CellNrMax;
};

#endif
