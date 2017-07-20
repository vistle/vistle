#include "moduleregistry.h"
#include "module.h"

namespace vistle {

ModuleRegistry::RegisterClass::RegisterClass(const std::string &name, NewModuleFunc factory) {

   ModuleRegistry::the().addModule(name, factory);
}

ModuleRegistry::ModuleClass::ModuleClass(const std::string &name, NewModuleFunc factory)
: name(name), newInstance(factory) {}

ModuleRegistry &ModuleRegistry::the() {

   static ModuleRegistry s_instance;
   return s_instance;
}

void ModuleRegistry::addModule(const std::string &name, NewModuleFunc factory) {
   auto added = m_modules.emplace(name, ModuleClass(name,factory)).second;
   if (!added) {
      std::cerr << "ModuleRegistry::addModule: duplicate module " << name << std::endl;
   }
   assert(added);
}

boost::function<ModuleRegistry::NewModuleFunc> ModuleRegistry::moduleFactory(const std::string &name) {

   boost::function<NewModuleFunc> f;

   auto it = m_modules.find(name);
   if (it == m_modules.end())
      return f;

   f = it->second.newInstance;

   return f;
}

std::shared_ptr<Module> ModuleRegistry::newInstance(const std::string &name, int id, mpi::communicator &comm) {

   auto it = m_modules.find(name);
   if (it == m_modules.end())
      return nullptr;

   return it->second.newInstance(name, id, comm);
}

bool ModuleRegistry::availableModules(AvailableMap &available) {

   const int hub = 0;

   for (const auto &m: m_modules) {

      AvailableModule mod;
      mod.hub = hub;
      mod.name = m.first;

      AvailableModule::Key key(hub, mod.name);
      available[key] = mod;
   }

   return true;
}

}
