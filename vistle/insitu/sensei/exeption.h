#ifndef VISTLE_SENSEI_EXEPTION_H
#define VISTLE_SENSEI_EXEPTION_H
#include <vistle/insitu/core/exeption.h>
namespace vistle {
namespace insitu {
namespace sensei {
struct Exeption: public InsituExeption {
    Exeption() { *this << "VistleSensei: "; };
};
} // namespace sensei
} // namespace insitu
} // namespace vistle
#endif
