#ifndef VISTLE_DIRECTORY_H
#define VISTLE_DIRECTORY_H

#include <string>
#include <map>

#include "export.h"

namespace vistle {

const int ModuleNameLength = 50;

struct AvailableModule {

   struct Key {
      int hub;
      std::string name;

      Key(int hub, const std::string &name)
         : hub(hub), name(name) {}

      bool operator<(const Key &rhs) const {

         if (hub < rhs.hub)
            return true;
         return name < rhs.name;
      }
   };

   int hub;
   std::string name;
   std::string path;

   AvailableModule(): hub(0) {}
};

typedef std::map<AvailableModule::Key, AvailableModule> AvailableMap;

V_UTILEXPORT bool scanModules(const std::string &directory, int hub, AvailableMap &available);

}

#endif
