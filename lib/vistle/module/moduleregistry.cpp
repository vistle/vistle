#include "moduleregistry.h"
#include "module.h"

#include <vistle/control/scanmodules.h>
#include <cmrc/cmrc.hpp>
#include <fstream>
#include <istream>
#include <streambuf>

CMRC_DECLARE(moduledescriptions);

namespace vistle {

ModuleRegistry::RegisterClass::RegisterClass(const std::string &name, NewModuleFunc factory)
{
    ModuleRegistry::the().addModule(name, factory);
}

ModuleRegistry::ModuleClass::ModuleClass(const std::string &name, NewModuleFunc factory)
: name(name), newInstance(factory)
{}

ModuleRegistry &ModuleRegistry::the()
{
    static ModuleRegistry s_instance;
    return s_instance;
}

void ModuleRegistry::addModule(const std::string &name, NewModuleFunc factory)
{
    auto added = m_modules.emplace(name, ModuleClass(name, factory)).second;
    if (!added) {
        std::cerr << "ModuleRegistry::addModule: duplicate module " << name << std::endl;
    }
    assert(added);
}

boost::function<ModuleRegistry::NewModuleFunc> ModuleRegistry::moduleFactory(const std::string &name)
{
    boost::function<NewModuleFunc> f;

    auto it = m_modules.find(name);
    if (it == m_modules.end())
        return f;

    f = it->second.newInstance;

    return f;
}

std::shared_ptr<Module> ModuleRegistry::newInstance(const std::string &name, int id, mpi::communicator &comm)
{
    auto it = m_modules.find(name);
    if (it == m_modules.end())
        return nullptr;

    return it->second.newInstance(name, id, comm);
}

bool ModuleRegistry::availableModules(AvailableMap &available, int hub)
{
    std::map<std::string, ModuleDescription> moduleDescriptions;
    try {
        auto fs = cmrc::moduledescriptions::get_filesystem();
        auto data = fs.open("moduledescriptions.txt");

        struct membuf: std::streambuf {
            membuf(const char *cbegin, const char *cend)
            {
                char *begin(const_cast<char *>(cbegin));
                char *end(const_cast<char *>(cend));
                this->setg(begin, begin, end);
            }
        };

        struct imemstream: virtual membuf, std::istream {
            imemstream(const char *begin, const char *end)
            : membuf(begin, end), std::istream(static_cast<std::streambuf *>(this))
            {}
        };

        imemstream str(data.begin(), data.end());
        moduleDescriptions = readModuleDescriptions(str);
    } catch (std::exception &ex) {
        std::cerr << "ModuleRegistry::availableModules: exception: " << ex.what() << std::endl;
    }

    for (const auto &m: m_modules) {
        std::string description;
        std::string category;
        auto &name = m.first;
        auto it = moduleDescriptions.find(name);
        if (it != moduleDescriptions.end()) {
            category = it->second.category;
            description = it->second.description;
        }
        AvailableModule::Key key(hub, name);

        available.emplace(key, AvailableModule{hub, name, "", category, description});
    }

    return true;
}

} // namespace vistle
