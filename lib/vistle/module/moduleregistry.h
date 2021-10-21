#ifndef VISTLE_MODULE_REGISTRY_H
#define VISTLE_MODULE_REGISTRY_H

#include "export.h"

#include <boost/function.hpp>
#include <boost/mpi.hpp>

#include <string>

#include <vistle/core/availablemodule.h>

namespace mpi = boost::mpi;

namespace vistle {

class Module;

class V_MODULEEXPORT ModuleRegistry {
    friend class RegisterClass;

public:
    typedef std::shared_ptr<vistle::Module>(NewModuleFunc)(const std::string &name, int id, mpi::communicator comm);
    class RegisterClass {
    public:
        RegisterClass(const std::string &name, NewModuleFunc factory);
    };

    static ModuleRegistry &the();
    std::shared_ptr<Module> newInstance(const std::string &name, int id, mpi::communicator &comm);
    boost::function<NewModuleFunc> moduleFactory(const std::string &name);

    bool availableModules(AvailableMap &available);


private:
    struct ModuleClass {
        ModuleClass(const std::string &name, NewModuleFunc factory);

        std::string name;
        NewModuleFunc *newInstance;
    };
    std::map<std::string, ModuleClass> m_modules;

    void addModule(const std::string &name, NewModuleFunc);
};

} // namespace vistle
#endif
