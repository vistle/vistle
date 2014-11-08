#ifndef CELLTOVERT_H
#define CELLTOVERT_H

#include <module/module.h>
#include <core/vector.h>

class CellToVert: public vistle::Module {

 public:
   CellToVert(const std::string &shmname, const std::string &name, int moduleID);
   ~CellToVert();

 private:

   virtual bool compute();
};

#endif
