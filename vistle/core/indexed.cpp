#include "indexed.h"
#include "celltree_impl.h"
#include "assert.h"
#include "archives.h"

namespace vistle {

Indexed::Indexed(const Index numElements, const Index numCorners,
                      const Index numVertices,
                      const Meta &meta)
   : Indexed::Base(static_cast<Data *>(NULL))
{
    refreshImpl();
}

bool Indexed::isEmpty() const {

   return getNumElements()==0 || getNumCorners()==0;
}

bool Indexed::checkImpl() const {

   V_CHECK (d()->el->check());
   V_CHECK (d()->cl->check());
   V_CHECK (d()->el->size() > 0);
   V_CHECK (el()[0] == 0);
   if (getNumElements() > 0) {
      V_CHECK (el()[getNumElements()-1] < getNumCorners());
      V_CHECK (el()[getNumElements()] == getNumCorners());
   }

   if (getNumCorners() > 0) {
      V_CHECK (cl()[0] < getNumVertices());
      V_CHECK (cl()[getNumCorners()-1] < getNumVertices());
   }

   return true;
}

std::pair<Vector, Vector> Indexed::getBounds() const {
   if (hasCelltree()) {
      const auto &ct = getCelltree();
      return std::make_pair(Vector(ct->min()), Vector(ct->max()));
   }

   return Base::getMinMax();
}

bool Indexed::hasCelltree() const {

   return hasAttachment("celltree");
}

Indexed::Celltree::const_ptr Indexed::getCelltree() const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(d()->attachment_mutex);
   if (!hasAttachment("celltree")) {
      refresh();
      createCelltree(getNumElements(), &el()[0], &cl()[0]);
   }

   Celltree::const_ptr ct = Celltree::as(getAttachment("celltree"));
   vassert(ct);
   return ct;
}

void Indexed::createCelltree(Index nelem, const Index *el, const Index *cl) const {

   if (hasCelltree())
      return;

   const Scalar *coords[3] = {
      &x()[0],
      &y()[0],
      &z()[0]
   };
   const Scalar smax = std::numeric_limits<Scalar>::max();
   Vector vmin, vmax;
   vmin.fill(-smax);
   vmax.fill(smax);

   std::vector<Vector> min(nelem, vmax);
   std::vector<Vector> max(nelem, vmin);

   Vector gmin=vmax, gmax=vmin;
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

   typename Celltree::ptr ct(new Celltree(nelem));
   ct->init(min.data(), max.data(), gmin, gmax);
   addAttachment("celltree", ct);
}


bool Indexed::hasVertexOwnerList() const {

   return hasAttachment("vertexownerlist");
}

Indexed::VertexOwnerList::const_ptr Indexed::getVertexOwnerList() const {

   boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> lock(d()->attachment_mutex);
   if (!hasAttachment("vertexownerlist")) {
      refresh();
      createVertexOwnerList();
   }

   VertexOwnerList::const_ptr vol = VertexOwnerList::as(getAttachment("vertexownerlist"));
   vassert(vol);
   return vol;
}

void Indexed::createVertexOwnerList() const {

   if (hasVertexOwnerList())
      return;

   Index numelem=getNumElements();
   Index numcoord=getNumVertices();

   VertexOwnerList::ptr vol(new VertexOwnerList(numcoord));
   const auto cl = &this->cl()[0];
   const auto el = &this->el()[0];
   auto vertexList=vol->vertexList().data();

   std::fill(vol->vertexList().begin(), vol->vertexList().end(), 0);
   std::vector<Index> used_vertex_list(numcoord, InvalidIndex);
   auto uvl = used_vertex_list.data();

   // Calculation of the number of cells that contain a certain vertex:
   // temporarily stored in vertexList
   for (Index i = 0; i < numelem; i++) {
      const Index begin = el[i], end = el[i+1];
      for (Index j = begin; j<end; ++j) {
         const Index v = cl[j];
         if (uvl[v] != j) {
             vertexList[v]++;
             uvl[v] = j;
         }
      }
   }

   // create the vertexList: prefix sum
   // vertexList will index into cellList
   std::vector<Index> outputIndex(numcoord);
   auto outIdx = outputIndex.data();
   {
      Index numEnt = 0;
      for (Index i = 0; i < numcoord; i++)
      {
         Index n = numEnt;
         numEnt += vertexList[i];
         vertexList[i] = n;
         outIdx[i] = n;
      }
      vertexList[numcoord] = numEnt;
      vol->cellList().resize(numEnt);
   }

   //fill the cellList
   std::fill(used_vertex_list.begin(), used_vertex_list.end(), InvalidIndex);
   auto cellList=vol->cellList().data();
   for (Index i = 0; i < numelem; i++) {
      const Index begin = el[i], end = el[i+1];
      for (Index j = begin; j<end; ++j) {
         const Index v = cl[j];
         if (uvl[v] != j) {
            uvl[v] = j;
            cellList[outIdx[v]] = i;
            outIdx[v]++;
         }
      }
   }

   addAttachment("vertexownerlist",vol);
}

void Indexed::removeVertexOwnerList() const {
    removeAttachment("vertexownerlist");
}

Indexed::NeighborFinder::NeighborFinder(const Indexed *indexed) {
    auto ol = indexed->getVertexOwnerList();
    numElem = indexed->getNumElements();
    numVert = indexed->getNumCorners();
    el = indexed->el();
    cl = indexed->cl();
    vl = ol->vertexList();
    vol = ol->cellList();
}

Index Indexed::NeighborFinder::getNeighborElement(Index elem, Index v1, Index v2, Index v3) {

   if (v1 == v2 || v1 == v3 || v2 == v3) {
      std::cerr << "WARNING: getNeighborElement was not called with 3 unique vertices." << std::endl;
      return InvalidIndex;
   }

   const Index *elems = &vol[vl[v1]];
   Index nvert = vl[v1+1] - vl[v1];
   for (Index j=0; j<nvert; ++j) {
       Index e = elems[j];
       if (e == elem)
           continue;
       bool f2=false, f3=false;
       for (Index i=el[e]; i<el[e+1]; ++i) {
           const Index v = cl[i];
           if (v == v2) {
               f2 = true;
               if (f3)
                   return e;
           } else if (v == v3) {
               f3 = true;
               if (f2)
                   return e;
           }
       }
   }
   return InvalidIndex;
}

Indexed::NeighborFinder Indexed::getNeighborFinder() const {

    return NeighborFinder(this);
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
   const auto cl = &this->cl()[0];
   const auto el = &this->el()[0];
   const Scalar *coords[3] = {
      &x()[0],
      &y()[0],
      &z()[0],
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

void Indexed::refreshImpl() const {

    const Data *d = static_cast<Data *>(m_data);
    m_el = (d && d->el.valid()) ? d->el->data() : nullptr;
    m_cl = (d && d->cl.valid()) ? d->cl->data() : nullptr;
}

Indexed::Data::Data(const Index numElements, const Index numCorners,
             const Index numVertices,
             Type id, const std::string & name,
             const Meta &meta)
   : Indexed::Base::Data(numVertices, id, name,
         meta)
{
   el.construct(numElements+1);
   cl.construct(numCorners);
   (*el)[0] = 0;
}

Indexed::Data::Data(const Indexed::Data &o, const std::string &name)
: Indexed::Base::Data(o, name)
, el(o.el)
, cl(o.cl)
{
}

Indexed::Data *Indexed::Data::create(const std::string &objId, Type id,
            const Index numElements, const Index numCorners,
            const Index numVertices,
            const Meta &meta) {

   // required for boost::serialization
   assert("should never be called" == NULL);

   return NULL;
}


Index Indexed::getNumElements() const {

   return d()->el->size()-1;
}

Index Indexed::getNumCorners() const {

   return d()->cl->size();
}

Index Indexed::getNumVertices() const {

    return getSize();
}

Object::Type Indexed::type() {
   vassert("should not be called" == 0);
   return Object::UNKNOWN;
}

V_SERIALIZERS(Indexed);
V_OBJECT_CTOR(Indexed);

} // namespace vistle
