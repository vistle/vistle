#include "vec.h"
#include "scalars.h"
#include "assert.h"
#include "indexed.h"
#include "celltree_impl.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace vistle {

bool DataBase::isEmpty() const {

   return Base::isEmpty();
}

bool DataBase::checkImpl() const {

   return true;
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
         createCelltree(g->getNumElements(), g->el().data(), g->cl().data());
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

DataBase::Data::Data(Type id, const std::string &name,
      const Meta &meta)
   : DataBase::Base::Data(id, name, meta)
   , grid(nullptr)
{
}

DataBase::Data::Data(const DataBase::Data &o, const std::string &n, Type id)
: DataBase::Base::Data(o, n, id)
, grid(o.grid)
{
   if (grid)
      grid->ref();
}


DataBase::Data::Data(const DataBase::Data &o, const std::string &n)
: DataBase::Base::Data(o, n)
, grid(o.grid)
{
   if (grid)
      grid->ref();
}

DataBase::Data::~Data() {

   if (grid)
      grid->unref();
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

   return Object::create(&*d()->grid);
}

void DataBase::setGrid(Object::const_ptr grid) {

   if (d()->grid)
      d()->grid->unref();
   d()->grid = grid->d();
   if (d()->grid)
      d()->grid->ref();
}

V_OBJECT_TYPE(DataBase, Object::DATABASE);


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
