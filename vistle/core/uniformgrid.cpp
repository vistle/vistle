//-------------------------------------------------------------------------
// UNIFORM GRID CLASS H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------

#include "uniformgrid.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::UniformGrid(Index xdim, Index ydim, Index zdim, const Meta &meta) : UniformGrid::Base(UniformGrid::Data::create(xdim, ydim, zdim, meta)) {
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void UniformGrid::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);

    for (int c=0; c<3; ++c) {
        m_numDivisions[c] = (*d->numDivisions)[c];
        m_min[c] = (*d->min)[c];
        m_max[c] = (*d->max)[c];
    }
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool UniformGrid::checkImpl() const {

   V_CHECK(d()->numDivisions->check());
   V_CHECK(d()->min->check());
   V_CHECK(d()->max->check());

   V_CHECK(d()->numDivisions->size() == 3);
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
UniformGrid::Data::Data(const std::string & name, Index xdim, Index ydim, Index zdim, const Meta &meta)
    : UniformGrid::Base::Data(Object::UNIFORMGRID, name, meta) {
    numDivisions.construct(3);
    min.construct(3);
    max.construct(3);

    (*numDivisions)[0] = xdim;
    (*numDivisions)[1] = ydim;
    (*numDivisions)[2] = zdim;
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
UniformGrid::Data::Data(const UniformGrid::Data &o, const std::string &n)
: UniformGrid::Base::Data(o, n) {
    numDivisions.construct(3);
    min.construct(3);
    max.construct(3);

    for (int i=0; i<3; ++i) {
        (*min)[i] = (*o.min)[i];
        (*max)[i] = (*o.max)[i];
        (*numDivisions)[i] = (*o.numDivisions)[i];
    }
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
UniformGrid::Data * UniformGrid::Data::create(Index xdim, Index ydim, Index zdim, const Meta &meta) {

    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(name, xdim, ydim, zdim, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(UniformGrid, Object::UNIFORMGRID)
V_OBJECT_CTOR(UniformGrid)

} // namespace vistle
