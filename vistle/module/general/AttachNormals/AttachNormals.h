#ifndef ATTACHNORMALS_H
#define ATTACHNORMALS_H

#include <module/module.h>
#include <core/vector.h>

class AttachNormals: public vistle::Module {

 public:
   AttachNormals(const std::string &name, int moduleID, mpi::communicator comm);
   ~AttachNormals();

 private:

   vistle::Port *m_gridIn, *m_dataIn, *m_gridOut;
   virtual bool compute();
};

#endif
