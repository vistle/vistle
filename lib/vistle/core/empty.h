#ifndef VISTLE_CORE_EMPTY_H
#define VISTLE_CORE_EMPTY_H

#include "shmname.h"
#include "object.h"
#include "archives_config.h"
#include "shm_obj_ref.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT Empty: public Object {
    V_OBJECT(Empty);

public:
    typedef Object Base;

    V_DATA_BEGIN(Empty);
    Data(const std::string &name, const Meta &m = Meta());
    ~Data();
    static Data *create(const std::string &name = "");
    V_DATA_END(Empty);
};

} // namespace vistle
#endif
