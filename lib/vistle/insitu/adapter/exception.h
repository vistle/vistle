#ifndef VISTLE_ADAPTER_EXCEPTION_H
#define VISTLE_ADAPTER_EXCEPTION_H
#include <vistle/insitu/core/exception.h>
namespace vistle {
namespace insitu {
struct Exception: public InsituException {
    Exception() { *this << "VistleSensei: "; };
};
} // namespace insitu
} // namespace vistle
#endif
