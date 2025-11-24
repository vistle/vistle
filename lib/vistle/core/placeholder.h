#ifndef VISTLE_CORE_PLACEHOLDER_H
#define VISTLE_CORE_PLACEHOLDER_H

#include "shmname.h"
#include "object.h"
#include "archives_config.h"
#include "shm_obj_ref.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT PlaceHolder: public Object {
    V_OBJECT(PlaceHolder);

public:
    typedef Object Base;

    PlaceHolder(const std::string &originalName, const Meta &originalMeta, const Object::Type originalType);
    PlaceHolder(Object::const_ptr obj);

    void setReal(Object::const_ptr g);
    Object::const_ptr real() const;
    std::string originalName() const;
    Object::Type originalType() const;
    const Meta &originalMeta() const;

    void setGeometry(Object::const_ptr geo);
    PlaceHolder::const_ptr geometry() const;
    void setNormals(Object::const_ptr norm);
    PlaceHolder::const_ptr normals() const;
    void setMapped(Object::const_ptr tex);
    PlaceHolder::const_ptr mapped() const;

    V_DATA_BEGIN(PlaceHolder);
    shm_obj_ref<Object> real;

    Data(const std::string &name, const std::string &originalName, const Meta &m, Object::Type originalType);
    ~Data();
    static Data *create(const std::string &name = "", const std::string &originalName = "",
                        const Meta &originalMeta = Meta(), const Object::Type originalType = Object::UNKNOWN);

private:
    shm_name_t originalName;
    Meta originalMeta;
    Object::Type originalType;

    shm_obj_ref<PlaceHolder> geometry, normals, mapped;
    V_DATA_END(PlaceHolder);
};

} // namespace vistle
#endif
