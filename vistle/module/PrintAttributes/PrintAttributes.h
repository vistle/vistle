#ifndef PRINTATTRIBUTES_H
#define PRINTATTRIBUTES_H

#include <module/module.h>

class PrintAttributes: public vistle::Module {

 public:
   PrintAttributes(const std::string &shmname, const std::string &name, int moduleID);
   ~PrintAttributes();

 private:
   virtual bool compute();
};

#endif
