#ifndef VISTLE_INFO_H
#define VISTLE_INFO_H

#include "export.h"
#include <vistle/module/module.h>

class V_PLUGINUTILEXPORT VistleInfo {
public:
    static void setModule(vistle::Module *mod);
    static vistle::Module *getModule();
};

#endif
