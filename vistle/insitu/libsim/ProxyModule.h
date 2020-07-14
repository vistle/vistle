#ifndef PROXY_MODULE_H
#define PROXY_MODULE_H

#include <string>
#include <vistle/module/module.h>

namespace insitu{
class ProxyModule : public vistle::Module{
public:
    ProxyModule(const std::string& shmName, const std::string& moduleName, int moduleID);



};
}

       



#endif // !PROXY_MODULE_H
