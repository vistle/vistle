#ifndef ATTACHGRID_H
#define ATTACHGRID_H

#include <module/module.h>
#include <core/vector.h>

class AttachGrid: public vistle::Module {

 public:
   AttachGrid(const std::string &shmname, const std::string &name, int moduleID);
   ~AttachGrid();

 private:

   vistle::Port *m_gridIn, *m_dataIn, *m_dataOut;
   virtual bool compute();
};

#endif
