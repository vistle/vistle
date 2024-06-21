#include "VistleInfo.h"

static vistle::Module *s_module = nullptr;

void VistleInfo::setModule(vistle::Module *mod)
{
    s_module = mod;
}
vistle::Module *VistleInfo::getModule()
{
    return s_module;
}
