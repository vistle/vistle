#ifndef TOPOLYHEDRA_H
#define TOPOLYHEDRA_H

#include <module/module.h>

class ToPolyhedra: public vistle::Module {

 public:
   ToPolyhedra(const std::string &shmname, const std::string &name, int moduleID);
   ~ToPolyhedra();

 private:
   virtual bool compute();
};

#endif
