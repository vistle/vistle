#ifndef VISTLE_INSITU_ADAPTER_EXCEPTION_H
#define VISTLE_INSITU_ADAPTER_EXCEPTION_H

#include <vistle/insitu/core/exception.h>
namespace vistle {
namespace insitu {
struct Exception: public InsituException {
    Exception() { *this << "VistleInSitu: "; };
};
} // namespace insitu
} // namespace vistle
#endif
