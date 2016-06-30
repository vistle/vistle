//-------------------------------------------------------------------------
// STRUCTURED GRID CLASS H
// *
// * Structured Grid Container Object
//-------------------------------------------------------------------------

#include "structuredgrid.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
StructuredGrid::StructuredGrid(const Index numVert_x, const Index numVert_y, const Index numVert_z, const Meta &meta)
    : StructuredGrid::Base(StructuredGrid::Data::create(numVert_x, numVert_y, numVert_z, meta)) {
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void StructuredGrid::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);

    m_numElements = (d && d->numElements.valid()) ? d->numElements->data() : nullptr;
    m_coords_x = (d && d->coords_x.valid()) ? d->coords_x->data() : nullptr;
    m_coords_y = (d && d->coords_y.valid()) ? d->coords_y->data() : nullptr;
    m_coords_z = (d && d->coords_z.valid()) ? d->coords_z->data() : nullptr;
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool StructuredGrid::checkImpl() const {

   V_CHECK(d()->numElements->check());
   V_CHECK(d()->coords_x->check());
   V_CHECK(d()->coords_y->check());
   V_CHECK(d()->coords_z->check());

   V_CHECK(d()->numElements->size() == 3);

   return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool StructuredGrid::isEmpty() const {

   return Base::isEmpty();
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
StructuredGrid::Data::Data(const Index numVert_x, const Index numVert_y, const Index numVert_z, const std::string & name, const Meta &meta)
    : StructuredGrid::Base::Data(Object::STRUCTUREDGRID, name, meta) {
    const Index numCoords = numVert_x * numVert_y * numVert_z;

    // construct ShmVectors
    numElements.construct(3);
    coords_x.construct(numCoords);
    coords_y.construct(numCoords);
    coords_z.construct(numCoords);

    // insert numElements
    numElements->data()[0] = numVert_x - 1;
    numElements->data()[1] = numVert_y - 1;
    numElements->data()[2] = numVert_z - 1;
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
StructuredGrid::Data::Data(const StructuredGrid::Data &o, const std::string &n)
: StructuredGrid::Base::Data(o, n)
, numElements(o.numElements)
, coords_x(o.coords_x)
, coords_y(o.coords_y)
, coords_z(o.coords_z) {
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
StructuredGrid::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
StructuredGrid::Data * StructuredGrid::Data::create(const Index numVert_x, const Index numVert_y, const Index numVert_z, const Meta &meta) {

    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numVert_x, numVert_y, numVert_z, name, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(StructuredGrid, Object::STRUCTUREDGRID)
V_OBJECT_CTOR(StructuredGrid)

} // namespace vistle
