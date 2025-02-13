#ifndef VISTLE_COVER_VISTLEPLUGINUTIL_VISTLEINFO_H
#define VISTLE_COVER_VISTLEPLUGINUTIL_VISTLEINFO_H

#include "export.h"
#include <vistle/module/module.h>

class V_PLUGINUTILEXPORT VistleInfo {
public:
    static void setModule(vistle::Module *mod);
    static vistle::Module *getModule();
};

#endif
