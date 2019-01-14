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

         if (hub == rhs.hub) {
             return name < rhs.name;
         }
         return hub < rhs.hub;
      }
   };

   int hub;
   std::string name;
   std::string path;

   AvailableModule(): hub(0) {}
};

typedef std::map<AvailableModule::Key, AvailableModule> AvailableMap;

V_UTILEXPORT bool scanModules(const std::string &directory, int hub, AvailableMap &available);

namespace directory {

V_UTILEXPORT std::string prefix(int argc, char *argv[]);
V_UTILEXPORT std::string prefix(const std::string &bindir);
V_UTILEXPORT std::string bin(const std::string &prefix);
V_UTILEXPORT std::string module(const std::string &prefix);
V_UTILEXPORT std::string share(const std::string &prefix);

} // namespace directory

}

#endif
