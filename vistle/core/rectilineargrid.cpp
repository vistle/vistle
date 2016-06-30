//-------------------------------------------------------------------------
// RECTILINEAR GRID CLASS H
// *
// * Rectilinear Grid Container Object
//-------------------------------------------------------------------------

#include "rectilineargrid.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
RectilinearGrid::RectilinearGrid(const Index size_x, const Index size_y, const Index size_z, const Meta &meta)
    : RectilinearGrid::Base(RectilinearGrid::Data::create(size_x, size_y, size_z, meta)) {
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void RectilinearGrid::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);

    m_coords_x = (d && d->coords_x.valid()) ? d->coords_x->data() : nullptr;
    m_coords_y = (d && d->coords_y.valid()) ? d->coords_y->data() : nullptr;
    m_coords_z = (d && d->coords_z.valid()) ? d->coords_z->data() : nullptr;
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool RectilinearGrid::checkImpl() const {

   V_CHECK(d()->coords_x->check());
   V_CHECK(d()->coords_y->check());
   V_CHECK(d()->coords_z->check());

   return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool RectilinearGrid::isEmpty() const {

   return (getSize_x() == 0 || getSize_y() == 0 || getSize_z() == 0);
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
RectilinearGrid::Data::Data(const Index size_x, const Index size_y, const Index size_z, const std::string & name, const Meta &meta)
    : RectilinearGrid::Base::Data(Object::RECTILINEARGRID, name, meta) {
    coords_x.construct(size_x);
    coords_y.construct(size_y);
    coords_z.construct(size_z);
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
RectilinearGrid::Data::Data(const RectilinearGrid::Data &o, const std::string &n)
: RectilinearGrid::Base::Data(o, n)
, coords_x(o.coords_x)
, coords_y(o.coords_y)
, coords_z(o.coords_z) {
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
RectilinearGrid::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
RectilinearGrid::Data * RectilinearGrid::Data::create(const Index size_x, const Index size_y, const Index size_z, const Meta &meta) {

    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(size_x, size_y, size_z, name, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(RectilinearGrid, Object::RECTILINEARGRID)
V_OBJECT_CTOR(RectilinearGrid)

} // namespace vistle
