#include "indexed.h"

namespace vistle {

Indexed::Indexed(const Index numElements, const Index numCorners,
                      const Index numVertices,
                      const Meta &meta)
   : Indexed::Base(static_cast<Data *>(NULL))
{
}

bool Indexed::isEmpty() const {

   return getNumElements()==0 || getNumCorners()==0;
}

bool Indexed::checkImpl() const {

   V_CHECK (el().size() > 0);
   if (getNumElements() > 0) {
      V_CHECK (el()[0] < getNumCorners());
      V_CHECK (el()[getNumElements()-1] < getNumCorners());
      V_CHECK (el()[getNumElements()] == getNumCorners());
   }

   if (getNumCorners() > 0) {
      V_CHECK (cl()[0] < getNumVertices());
      V_CHECK (cl()[getNumCorners()-1] < getNumVertices());
   }

   return true;
}

bool Indexed::hasCelltree() const {

   return hasAttachment("celltree");
}

Indexed::Celltree::const_ptr Indexed::getCelltree() const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(d()->attachment_mutex);
   Celltree::const_ptr ct;
   if (!hasAttachment("celltree"))
      createCelltree();

   ct = Celltree::as(getAttachment("celltree"));
   assert(ct);
   return ct;
}

void Indexed::createCelltree() const {

   if (hasCelltree())
      return;

   const Scalar *coords[3] = {
      x().data(),
      y().data(),
      z().data()
   };
   const Scalar smax = std::numeric_limits<Scalar>::max();

   const Index nelem = getNumElements();
   std::vector<Vector> min(nelem, Vector(smax, smax, smax));
   std::vector<Vector> max(nelem, Vector(-smax, -smax, -smax));
   auto cl = this->cl().data();
   auto el = this->el().data();

   Vector gmin(smax, smax, smax), gmax(-smax, -smax, -smax);
   for (Index i=0; i<nelem; ++i) {
      const Index start = el[i], end = el[i+1];
      for (Index c = start; c<end; ++c) {
         const Index v = cl[c];
         for (int d=0; d<3; ++d) {
            if (min[i][d] > coords[d][v]) {
               min[i][d] = coords[d][v];
               if (gmin[d] > min[i][d])
                  gmin[d] = min[i][d];
            }
            if (max[i][d] < coords[d][v]) {
               max[i][d] = coords[d][v];
               if (gmax[d] < max[i][d])
                  gmax[d] = max[i][d];
            }
         }
      }
   }

   Celltree::ptr ct(new Celltree(getNumElements()));
   ct->init(min.data(), max.data(), gmin, gmax);
   addAttachment("celltree", ct);
}



Indexed::Data::Data(const Index numElements, const Index numCorners,
             const Index numVertices,
             Type id, const std::string & name,
             const Meta &meta)
   : Indexed::Base::Data(numVertices, id, name,
         meta)
   , el(new ShmVector<Index>(numElements+1))
   , cl(new ShmVector<Index>(numCorners))
{
}

Indexed::Data::Data(const Indexed::Data &o, const std::string &name)
: Indexed::Base::Data(o, name)
, el(o.el)
, cl(o.cl)
{
}

Indexed::Data *Indexed::Data::create(Type id,
            const Index numElements, const Index numCorners,
            const Index numVertices,
            const Meta &meta) {

   // required for boost::serialization
   assert("should never be called" == NULL);

   return NULL;
}


Index Indexed::getNumElements() const {

   return el().size()-1;
}

Index Indexed::getNumCorners() const {

   return cl().size();
}

Index Indexed::getNumVertices() const {

   return x(0).size();
}

V_SERIALIZERS(Indexed);

} // namespace vistle
