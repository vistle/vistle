#ifndef METADATA_H
#define METADATA_H

#include <module/module.h>
#include <core/vector.h>

class MetaData: public vistle::Module {

 public:
   MetaData(const std::string &shmname, const std::string &name, int moduleID);
   ~MetaData();

 private:

   std::map<int, vistle::Object::const_ptr> m_objs;
   virtual bool compute();

   vistle::IntParameter *m_kind;
   vistle::IntParameter *m_modulus;
   vistle::IntVectorParameter *m_range;
};

#endif
