#ifndef EXTRACTGRID_H
#define EXTRACTGRID_H

#include <module/module.h>
#include <core/vector.h>

class ExtractGrid: public vistle::Module {

 public:
   ExtractGrid(const std::string &shmname, const std::string &name, int moduleID);
   ~ExtractGrid();

 private:

   vistle::Port *m_dataIn, *m_gridOut, *m_normalsOut;
   virtual bool compute();
};

#endif
