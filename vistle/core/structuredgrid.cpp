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

    for (int c=0; c<3; ++c) {
        if (d && d->x[c].valid()) {
            m_numDivisions[c] = (*d->numDivisions)[c];
            m_x[c] = d->x[c]->data();
        } else {
            m_numDivisions[c] = 0;
            m_x[c] = nullptr;
        }
    }
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool StructuredGrid::checkImpl() const {

   for (int c=0; c<3; ++c)
       V_CHECK(d()->x[c]->check());

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
    : StructuredGrid::Base::Data(Object::STRUCTUREDGRID, name, meta)
{
    numDivisions.construct(3);
    (*numDivisions)[0] = numVert_x;
    (*numDivisions)[1] = numVert_y;
    (*numDivisions)[2] = numVert_z;

    const Index numCoords = numVert_x * numVert_y * numVert_z;
    // construct ShmVectors
    for (int c=0; c<3; ++c) {
        x[c].construct(numCoords);
    }
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
StructuredGrid::Data::Data(const StructuredGrid::Data &o, const std::string &n)
    : StructuredGrid::Base::Data(o, n)
    , numDivisions(o.numDivisions) {
    for (int c=0; c<3; ++c)
        x[c] = o.x[c];
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
StructuredGrid::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
StructuredGrid::Data * StructuredGrid::Data::create(const Index numVert_x, const Index numVert_y, const Index numVert_z, const Meta &meta) {

    // construct shm data
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
