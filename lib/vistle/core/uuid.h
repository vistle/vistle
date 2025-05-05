#ifndef VISTLE_CORE_UUID_H
#define VISTLE_CORE_UUID_H

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>

#include "export.h"

namespace vistle {
class V_COREEXPORT Uuid {
public:
    static boost::uuids::uuid generate();
};
} // namespace vistle

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
