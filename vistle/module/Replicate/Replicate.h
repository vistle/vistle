#ifndef REPLICATE_H
#define REPLICATE_H

#include <module/module.h>
#include <core/vector.h>

class Replicate: public vistle::Module {

 public:
   Replicate(const std::string &shmname, int rank, int size, int moduleID);
   ~Replicate();

 private:

   std::map<int, vistle::Object::const_ptr> m_objs;
   virtual bool compute();
};

#endif
