#include "vec.h"
#include "scalars.h"
#include "assert.h"
#include "indexed.h"
#include "triangles.h"
#include "celltree_impl.h"
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
   if (hasCelltree()) {
       V_CHECK(getCelltree()->check());
   }
   return true;
}

void DataBase::copyAttributes(Object::const_ptr src, bool replace) {
    Base::copyAttributes(src, replace);
    if (auto d = DataBase::as(src)) {
        setMapping(d->mapping());
    }
}

bool DataBase::hasCelltree() const {

   return hasAttachment("celltree");
}

Object::const_ptr DataBase::getCelltree() const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(d()->attachment_mutex);
   if (!hasAttachment("celltree")) {
      if (!grid()) {
         return Object::ptr();
      }

      if (auto g = Indexed::as(grid())) {
         createCelltree(g->getNumElements(), &g->el()[0], &g->cl()[0]);
      }
   }

   Object::const_ptr ct = getAttachment("celltree");
   vassert(ct);
   return ct;
}

void DataBase::createCelltree(Index nelem, const Index *el, const Index *cl) const {
   (void)nelem;
   (void)el;
   (void)cl;
}

#if 0
DataBase::Data::Data(Type id, const std::string &name,
      const Meta &meta)
   : DataBase::Base::Data(id, name, meta)
{
}
#endif

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
        }
    }
    return mapping();
}

//V_OBJECT_TYPE(DataBase, Object::DATABASE);
V_OBJECT_CTOR(DataBase);


V_SERIALIZERS4(Vec<T,Dim>, template<class T,int Dim>);

namespace mpl = boost::mpl;

namespace {

template<int Dim>
struct instantiator {
   template <typename V> void operator()(V) {
      typedef Vec<V, Dim> VEC;
      auto vec = new VEC(0, Meta());
      vec->setSize(1);
      vec->createCelltree(0, nullptr, nullptr);
      Index size = 0;
      auto ct = new typename Vec<V, Dim>::Celltree(size);
      
      typename VEC::Vector v;
      ct->init(&v, &v, v, v);
   }
};

template<int Dim>
struct registrator {
   template <typename S> void operator()(S) {
      typedef Vec<S,Dim> V;
      typedef typename mpl::find<Scalars, S>::type iter;
      ObjectTypeRegistry::registerType<V>(V::type());
      boost::serialization::void_cast_register<V, typename V::Base>(
            static_cast<V *>(NULL), static_cast<typename V::Base *>(NULL)
            );

   }
};

struct DoIt {
   DoIt() {
      mpl::for_each<Scalars>(registrator<1>());
      mpl::for_each<Scalars>(registrator<3>());
   }
};

static DoIt doit;

}

void inst_vecs() {

   mpl::for_each<Scalars>(instantiator<1>());
   mpl::for_each<Scalars>(instantiator<3>());
}

} // namespace vistle
