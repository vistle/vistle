#ifndef LOADCOVER_H
#define LOADCOVER_H

#include <module/module.h>

class LoadCover: public vistle::Module {

 public:
   LoadCover(const std::string &shmname, const std::string &name, int moduleID);
   ~LoadCover();

 private:

   bool prepare() override;
   bool compute() override;
};

#endif
