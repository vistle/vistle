#ifndef READVISTLE_H
#define READVISTLE_H

#include <string>
#include <module/module.h>

class ReadVistle: public vistle::Module {

 public:
   ReadVistle(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadVistle();

 private:
   bool load(const std::string & name);
   virtual bool compute();
};

#endif
