#include "indexed.h"
#include "celltree_impl.h"
#include "assert.h"

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
      V_CHECK (el()[0] == 0);
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
   if (!hasAttachment("celltree"))
      createCelltree();

   Celltree::const_ptr ct = Celltree::as(getAttachment("celltree"));
   vassert(ct);
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
   const auto cl = this->cl().data();
   const auto el = this->el().data();

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

bool Indexed::hasVertexOwnerList() const {

   return hasAttachment("vertexownerlist");
}

Indexed::VertexOwnerList::const_ptr Indexed::getVertexOwnerList() const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(d()->attachment_mutex);
   if (!hasAttachment("vertexownerlist"))
      createVertexOwnerList();

   VertexOwnerList::const_ptr vol = VertexOwnerList::as(getAttachment("vertexownerlist"));
   vassert(vol);
   return vol;
}

void Indexed::createVertexOwnerList() const {

   if (hasVertexOwnerList())
      return;

   Index numelem, numcoord;

   numelem=getNumElements();
   numcoord=getNumVertices();

   VertexOwnerList::ptr vol(new VertexOwnerList(numcoord));
   std::vector<Index> tmpl1(numcoord);
   const auto cl = this->cl().data();
   const auto el = this->el().data();
   auto vertexList=vol->vertexList().data();

   std::memset(vertexList, 0, (numcoord+1) * sizeof(Index));

   // Calculation of the number of cells that contain a certain vertex
   std::vector<Index> used_vertex_list(numcoord, InvalidIndex);

   for (Index i = 0; i < numelem; i++)
   {
      for (Index j = el[i]; j < el[i+1]; j++)
      {
         if (used_vertex_list[cl[j]] != i)
         {
            used_vertex_list[cl[j]] = i;
            vertexList[cl[j]]++;
         }
      }
   }

   //create the vertexList
   {
      Index j=0;
      for (Index i = 0; i < numcoord; i++)
      {
         Index ja = j;
         j += vertexList[i];
         vertexList[i] = ja;
         tmpl1[i] = ja;
      }
      vertexList[numcoord] = j;
      vol->cellList().resize(j);
   }
   auto cellList=vol->cellList().data();
   std::memset(used_vertex_list.data(), 0xff, numcoord * sizeof(Index));

   //fill the cellList
   for (Index i = 0; i < numelem; i++)
   {
      for (Index j = el[i]; j < el[i+1]; j++)
      {
         Index clj = cl[j];
         if (used_vertex_list[clj] != i)
         {
            used_vertex_list[clj] = i;
            cellList[tmpl1[clj]] = i;
            tmpl1[clj]++;
         }
      }
   }

   addAttachment("vertexownerlist",vol);
}

void Indexed::removeVertexOwnerList() const {
   removeAttachment("vertexownerlist");
}

struct CellBoundsFunctor: public Indexed::Celltree::CellBoundsFunctor {

   CellBoundsFunctor(const Indexed *indexed)
      : m_indexed(indexed)
      {}

   bool operator()(Index elem, Vector *min, Vector *max) const {

      return m_indexed->getElementBounds(elem, min, max);
   }

   const Indexed *m_indexed;
};

bool Indexed::validateCelltree() const {

   if (!hasCelltree())
      return false;

   CellBoundsFunctor boundFunc(this);
   auto ct = getCelltree();
   return ct->validateTree(boundFunc);
}

bool Indexed::getElementBounds(Index elem, Vector *min, Vector *max) const {

   const Scalar smax = std::numeric_limits<Scalar>::max();
   *min = Vector(smax, smax, smax);
   *max = Vector(-smax, -smax, -smax);
   const auto cl = this->cl().data();
   const auto el = this->el().data();
   const Scalar *coords[3] = {
      x().data(),
      y().data(),
      z().data()
   };

   const Index begin = el[elem], end = el[elem+1];
   for (Index c=begin; c<end; ++c) {
      const Index v = cl[c];
      for (int d=0; d<3; ++d) {
         if ((*min)[d] > coords[d][v]) {
            (*min)[d] = coords[d][v];
         }
         if ((*max)[d] < coords[d][v]) {
            (*max)[d] = coords[d][v];
         }
      }
   }

   return true;
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
