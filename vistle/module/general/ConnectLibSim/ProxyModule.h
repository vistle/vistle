#ifndef PROXY_MODULE_H
#define PROXY_MODULE_H

#include <string>
#include <module/module.h>

namespace in_situ{
class ProxyModule : public vistle::Module{
public:
    ProxyModule(const std::string& shmName, const std::string& moduleName, int moduleID);



};
}

       



#endif // !PROXY_MODULE_H
