//-------------------------------------------------------------------------
// UNIFORM GRID CLASS H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------

#include "uniformgrid.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::UniformGrid(const Meta &meta) : UniformGrid::Base(UniformGrid::Data::create(meta)) {
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void UniformGrid::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);

    m_size = (d && d->size.valid()) ? d->size->data() : nullptr;
    m_min = (d && d->min.valid()) ? d->min->data() : nullptr;
    m_max = (d && d->max.valid()) ? d->max->data() : nullptr;
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool UniformGrid::checkImpl() const {

   V_CHECK(d()->size->check());
   V_CHECK(d()->min->check());
   V_CHECK(d()->max->check());

   V_CHECK(d()->size->size() == 3);
   V_CHECK(d()->min->size() == 3);
   V_CHECK(d()->max->size() == 3);

   return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool UniformGrid::isEmpty() const {

   return Base::isEmpty();
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
UniformGrid::Data::Data(const std::string & name, const Meta &meta)
    : UniformGrid::Base::Data(Object::UNIFORMGRID, name, meta) {
    size.construct(3);
    min.construct(3);
    max.construct(3);
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
UniformGrid::Data::Data(const UniformGrid::Data &o, const std::string &n)
: UniformGrid::Base::Data(o, n)
, size(o.size)
, min(o.min)
, max(o.max) {
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
UniformGrid::Data * UniformGrid::Data::create(const Meta &meta) {

    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(name, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(UniformGrid, Object::UNIFORMGRID)
V_OBJECT_CTOR(UniformGrid)

} // namespace vistle
