//-------------------------------------------------------------------------
// RECTILINEAR GRID CLASS H
// *
// * Rectilinear Grid Container Object
//-------------------------------------------------------------------------

#include "rectilineargrid.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
RectilinearGrid::RectilinearGrid(const Index NumElements_x, const Index NumElements_y, const Index NumElements_z, const Meta &meta)
    : RectilinearGrid::Base(RectilinearGrid::Data::create(NumElements_x, NumElements_y, NumElements_z, meta)) {
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void RectilinearGrid::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);
    for (int c=0; c<3; ++c) {
        m_coords[c] = (d && d->coords[c].valid()) ? d->coords[c]->data() : nullptr;
    }
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool RectilinearGrid::checkImpl() const {

    for (int c=0; c<3; ++c) {
        V_CHECK(d()->coords[c]->check());
    }

   return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool RectilinearGrid::isEmpty() const {

   return (getNumDivisions(0) == 0 || getNumDivisions(1) == 0 || getNumDivisions(2) == 0);
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
RectilinearGrid::Data::Data(const Index NumElements_x, const Index NumElements_y, const Index NumElements_z, const std::string & name, const Meta &meta)
    : RectilinearGrid::Base::Data(Object::RECTILINEARGRID, name, meta) {
    coords[0].construct(NumElements_x + 1);
    coords[1].construct(NumElements_y + 1);
    coords[2].construct(NumElements_z + 1);
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
RectilinearGrid::Data::Data(const RectilinearGrid::Data &o, const std::string &n)
    : RectilinearGrid::Base::Data(o, n) {
    for (int c=0; c<3; ++c) {
        coords[c] = o.coords[c];
    }
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
RectilinearGrid::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
RectilinearGrid::Data * RectilinearGrid::Data::create(const Index NumElements_x, const Index NumElements_y, const Index NumElements_z, const Meta &meta) {

    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(NumElements_x, NumElements_y, NumElements_z, name, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(RectilinearGrid, Object::RECTILINEARGRID)
V_OBJECT_CTOR(RectilinearGrid)

} // namespace vistle
