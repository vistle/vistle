#include "vec.h"
#include "scalars.h"

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

Object::Type DataBase::type()  {

    return DATABASE;
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

V_SERIALIZERS(DataBase);


V_SERIALIZERS4(Vec<T,Dim>, template<class T,int Dim>);

namespace mpl = boost::mpl;

namespace {

template<int Dim>
struct instantiator {
   template <typename V> void operator()(V) {
      auto v = new Vec<V, Dim>(0, Meta());
      v->setSize(1);
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
