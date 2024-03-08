#include "scalars.h"
#include <cassert>
#include "database.h"
#include "database_impl.h"
#include "geometry.h"
#include "coords.h"
#include "archives.h"
#include "validate.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace vistle {

bool DataBase::isEmpty()
{
    return Base::isEmpty();
}

bool DataBase::isEmpty() const
{
    return Base::isEmpty();
}

void DataBase::refreshImpl() const
{}

bool DataBase::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_ENUM(mapping());
    VALIDATE_SUB(grid());

    return true;
}

void DataBase::copyAttributes(Object::const_ptr src, bool replace)
{
    Base::copyAttributes(src, replace);
    if (replace || mapping() == DataBase::Unspecified) {
        if (auto d = DataBase::as(src)) {
            setMapping(d->mapping());
        }
    }
}

void DataBase::Data::initData()
{
    mapping = DataBase::Unspecified;
}

DataBase::Data::Data(Type id, const std::string &name, const Meta &meta)
: DataBase::Base::Data(id, name, meta), mapping(DataBase::Unspecified)
{}

DataBase::Data::Data(const DataBase::Data &o, const std::string &n, Type id)
: DataBase::Base::Data(o, n, id), grid(o.grid), mapping(o.mapping)
{}


DataBase::Data::Data(const DataBase::Data &o, const std::string &n)
: DataBase::Base::Data(o, n), grid(o.grid), mapping(DataBase::Unspecified)
{}

DataBase::Data::~Data()
{}

DataBase::Data *DataBase::Data::create(Type id, const Meta &meta)
{
    assert("should never be called" == NULL);

    return NULL;
}

std::set<Object::const_ptr> DataBase::referencedObjects() const
{
    auto objs = Base::referencedObjects();

    if (grid()) {
        auto go = grid()->referencedObjects();
        std::copy(go.begin(), go.end(), std::inserter(objs, objs.begin()));
        objs.emplace(grid());
    }

    return objs;
}

void DataBase::resetArrays()
{
    assert("should never be called" == NULL);
}

Index DataBase::getSize()
{
    assert("should never be called" == NULL);

    return 0;
}

Index DataBase::getSize() const
{
    assert("should never be called" == NULL);

    return 0;
}

void DataBase::setSize(size_t size)
{
    assert("should never be called" == NULL);
}

void DataBase::applyDimensionHint(Object::const_ptr grid)
{}

Object::const_ptr DataBase::grid() const
{
    return d()->grid.getObject();
}

void DataBase::setGrid(Object::const_ptr grid)
{
    assert(!grid || grid->check(std::cerr));
    d()->grid = grid;
    applyDimensionHint(grid);
}

void DataBase::setMapping(DataBase::Mapping mapping)
{
    d()->mapping = mapping;
}

void DataBase::setExact(bool enable)
{}

DataBase::Mapping DataBase::mapping() const
{
    return d()->mapping;
}

DataBase::Mapping DataBase::guessMapping(Object::const_ptr g) const
{
    if (!g)
        g = grid();
    if (g && mapping() == Unspecified) {
        if (auto e = g->getInterface<ElementInterface>()) {
            if (getSize() == e->getNumVertices())
                return Vertex;
            else if (getSize() == e->getNumElements())
                return Element;
        } else if (auto coords = Coords::as(g)) {
            if (getSize() == coords->getSize())
                return Vertex;
        }
    }
    return mapping();
}

unsigned DataBase::dimension() const
{
    assert("should never be called" == NULL);
    return 0;
}

bool DataBase::copyEntry(Index to, DataBase::const_ptr src, Index from)
{
    assert("should never be called" == NULL);
    return false;
}

void DataBase::setValue(Index idx, unsigned component, const double &value)
{
    assert("should never be called" == NULL);
}

double DataBase::value(Index idx, unsigned component) const
{
    assert("should never be called" == NULL);
    return 0;
}

void DataBase::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
    if (!getInterface<GeometryInterface>()) {
        os << " map:" << toString(mapping());
        os << " grid(";
        if (grid()) {
            grid()->print(os, verbose);
        }
        os << ")";
    }
}

V_OBJECT_TYPE(DataBase, Object::DATABASE)
//V_OBJECT_CTOR(DataBase)
DataBase::DataBase(DataBase::Data *data): DataBase::Base(data)
{
    refreshImpl();
}
DataBase::DataBase(): DataBase::Base()
{
    refreshImpl();
}
#if 0
//V_OBJECT_CREATE_NAMED(DataBase)
#else
DataBase::Data *DataBase::Data::createNamed(Object::Type id, const std::string &name)
{
    Data *t = nullptr;
    t = shm<Data>::construct(name)(id, name, Meta());
    publish(t);
    return t;
}
#endif

V_OBJECT_IMPL(DataBase)
V_OBJECT_INST(DataBase)

} // namespace vistle
