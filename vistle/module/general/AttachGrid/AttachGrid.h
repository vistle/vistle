#ifndef ATTACHGRID_H
#define ATTACHGRID_H

#include <module/module.h>
#include <core/vector.h>

class AttachGrid: public vistle::Module {

 public:
   AttachGrid(const std::string &name, int moduleID, mpi::communicator comm);
   ~AttachGrid();

 private:

   vistle::Port *m_gridIn;
   std::vector<vistle::Port *> m_dataIn, m_dataOut;
   virtual bool compute();
};

#endif
