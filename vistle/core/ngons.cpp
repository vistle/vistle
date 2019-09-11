#include "ngons.h"
#include "triangles.h"
#include "quads.h"

namespace vistle {

template<int N>
Ngons<N>::Ngons(const Index numCorners, const Index numCoords,
                     const Meta &meta)
   : Ngons::Base(Ngons::Data::create(numCorners, numCoords,
            meta)) {
    refreshImpl();
}

template<int N>
void Ngons<N>::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);
    m_cl = (d && d->cl.valid()) ? d->cl->data() : nullptr;
    m_numCorners = (d && d->cl.valid()) ? d->cl->size() : 0;
}

template<int N>
bool Ngons<N>::isEmpty() {

   return getNumCoords()==0;
}

template<int N>
bool Ngons<N>::isEmpty() const {

   return getNumCoords()==0;
}

template<int N>
bool Ngons<N>::checkImpl() const {

   V_CHECK (d()->cl->check());
   if (getNumCorners() > 0) {
      V_CHECK (cl()[0] < getNumVertices());
      V_CHECK (cl()[getNumCorners()-1] < getNumVertices());
      V_CHECK (getNumCorners() % N == 0);
   } else {
      V_CHECK (getNumCoords() % N == 0);
   }
   return true;
}

template<int N>
void Ngons<N>::Data::initData() {
}

template<int N>
Ngons<N>::Data::Data(const Ngons::Data &o, const std::string &n)
: Ngons::Base::Data(o, n)
, cl(o.cl)
{
   initData();
}

template<int N>
Ngons<N>::Data::Data(const Index numCorners, const Index numCoords,
                     const std::string & name,
                     const Meta &meta)
    : Base::Data(numCoords,
                 N == 3 ? Object::TRIANGLES : Object::QUADS, name,
         meta)
{
   initData();
   cl.construct(numCorners);
}


template<int N>
typename Ngons<N>::Data * Ngons<N>::Data::create(const Index numCorners,
                              const Index numCoords,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectId();
   Data *t = shm<Data>::construct(name)(numCorners, numCoords, name, meta);
   publish(t);

   return t;
}

template<int N>
Index Ngons<N>::getNumElements() {

    return getNumCorners()>0 ? getNumCorners()/N : getNumCoords()/N;
}

template<int N>
Index Ngons<N>::getNumElements() const {

    return getNumCorners()>0 ? getNumCorners()/N : getNumCoords()/N;
}

template<int N>
Index Ngons<N>::getNumCorners() {

   return d()->cl->size();
}

template<int N>
Index Ngons<N>::getNumCorners() const {

   return m_numCorners;
}

template <int N>
Ngons<N>::Ngons(Data *data)
    : Ngons::Base(data)
{
    refreshImpl();
}

#if 0
V_OBJECT_CREATE_NAMED(Vec<T,Dim>)
#else
template<int N>
Ngons<N>::Data::Data(Object::Type id, const std::string &name, const Meta &m)
: Ngons<N>::Base::Data(id, name, m)
{
   initData();
}

template<int N>
typename Ngons<N>::Data *Ngons<N>::Data::createNamed(Object::Type id, const std::string &name) {
   Data *t = shm<Data>::construct(name)(id, name);
   publish(t);
   return t;
}
#endif
//V_OBJECT_CTOR(Triangles)

template<int N>
Object::Type Ngons<N>::type() {
    return N == 3 ? Object::TRIANGLES : Object::QUADS;
}

template class Ngons<3>;
template class Ngons<4>;


#if 0
//V_OBJECT_TYPE(Triangles, Object::TRIANGLES)

//V_OBJECT_TYPE(Quads, Object::QUADS)
V_OBJECT_CTOR(Quads)
#endif

} // namespace vistle
