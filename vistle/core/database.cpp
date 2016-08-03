#include "scalars.h"
#include "assert.h"
#include "indexed.h"
#include "triangles.h"
#include "structuredgridbase.h"
#include "archives.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace vistle {

bool DataBase::isEmpty() const {

   return Base::isEmpty();
}

void DataBase::refreshImpl() const {
}

bool DataBase::checkImpl() const {

   if (grid()) {
       V_CHECK(grid()->check());
   }

   V_CHECK(mapping()==Unspecified || mapping()==Vertex || mapping()==Element);
   return true;
}

void DataBase::copyAttributes(Object::const_ptr src, bool replace) {
    Base::copyAttributes(src, replace);
    if (auto d = DataBase::as(src)) {
        setMapping(d->mapping());
    }
}

DataBase::Data::Data(Type id, const std::string &name, const Meta &meta)
   : DataBase::Base::Data(id, name, meta)
   , mapping(DataBase::Unspecified)
{
}

DataBase::Data::Data(const DataBase::Data &o, const std::string &n, Type id)
: DataBase::Base::Data(o, n, id)
, grid(o.grid)
, mapping(o.mapping)
{
}


DataBase::Data::Data(const DataBase::Data &o, const std::string &n)
: DataBase::Base::Data(o, n)
, grid(o.grid)
, mapping(DataBase::Unspecified)
{
}

DataBase::Data::~Data() {

}

DataBase::Data *DataBase::Data::create(Type id, const Meta &meta) {

   assert("should never be called" == NULL);

   return NULL;
}

Index DataBase::getSize() const {

   assert("should never be called" == NULL);

   return 0;
}

void DataBase::setSize(Index size ) {

   assert("should never be called" == NULL);
}

Object::const_ptr DataBase::grid() const {

   return d()->grid.getObject();
}

void DataBase::setGrid(Object::const_ptr grid) {

    d()->grid = grid;
}

void DataBase::setMapping(DataBase::Mapping mapping) {
    d()->mapping = mapping;
}

DataBase::Mapping DataBase::mapping() const {
    return d()->mapping;
}

DataBase::Mapping DataBase::guessMapping(Object::const_ptr g) const {
    if (!g)
        g = grid();
    if (mapping() == Unspecified) {
        if (auto i = Indexed::as(g)) {
            if (getSize() == i->getNumVertices())
                return Vertex;
            else if (getSize() == i->getNumElements())
                return Element;
        } else if (auto t = Triangles::as(g)) {
            if (getSize() == t->getNumVertices())
                return Vertex;
            else if (getSize() == t->getNumElements())
                return Element;
        } else if (auto s = g->getInterface<StructuredGridBase>()) {
            if (getSize() == s->getNumDivisions(0)*s->getNumDivisions(1)*s->getNumDivisions(2))
                return Vertex;
            else if (getSize() == s->getNumElements())
                return Element;
        }
    }
    return mapping();
}

//V_OBJECT_TYPE(DataBase, Object::DATABASE);
//V_OBJECT_CTOR(DataBase);
DataBase::DataBase(DataBase::Data *data): DataBase::Base(data) { refreshImpl(); }
DataBase::DataBase(): DataBase::Base() { refreshImpl(); }

} // namespace vistle
