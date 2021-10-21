#ifndef VISTLE_SENSEI_EXCEPTION_H
#define VISTLE_SENSEI_EXCEPTION_H
#include <vistle/insitu/core/exception.h>
namespace vistle {
namespace insitu {
namespace sensei {
struct Exception: public InsituException {
    Exception() { *this << "VistleSensei: "; };
};
} // namespace sensei
} // namespace insitu
} // namespace vistle
#endif
