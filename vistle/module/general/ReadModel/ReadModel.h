#ifndef READMODEL_H
#define READMODEL_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/module.h>

class ReadModel: public vistle::Module {

 public:
   ReadModel(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadModel();

 private:

   bool load(const std::string & filename);

   virtual bool compute();
};

#endif
