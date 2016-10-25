//-------------------------------------------------------------------------
// UNIFORM GRID CLASS H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------

#include "uniformgrid.h"
#include "assert.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::UniformGrid(Index xdim, Index ydim, Index zdim, const Meta &meta)
: UniformGrid::Base(UniformGrid::Data::create(xdim, ydim, zdim, meta))
{
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void UniformGrid::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);

    for (int c=0; c<3; ++c) {
        m_numDivisions[c] = d ? (*d->numDivisions)[c] : 1;
        m_min[c] = d ? (*d->min)[c] : 0.f;
        m_max[c] = d ? (*d->max)[c] : 0.f;
        m_dist[c] = (m_max[c]-m_min[c])/(m_numDivisions[c]-1);

        m_ghostLayers[c][0] = d ? (*d->ghostLayers[c])[0] : 0;
        m_ghostLayers[c][1] = d ? (*d->ghostLayers[c])[1] : 0;
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

   for (int c=0; c<3; c++) {
       V_CHECK(d()->ghostLayers[c]->check());
        V_CHECK(d()->ghostLayers[c]->size() == 2);
   }

   return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool UniformGrid::isEmpty() const {

   return Base::isEmpty();
}

// GET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
Index UniformGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos) {
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return (*d()->ghostLayers[dim])[layerPosition];
}

// GET FUNCTION - GHOST CELL LAYER CONST
//-------------------------------------------------------------------------
Index UniformGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const {
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return m_ghostLayers[dim][layerPosition];
}

// SET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
void UniformGrid::setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) {
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    (*d()->ghostLayers[dim])[layerPosition] = value;
    m_ghostLayers[dim][layerPosition] = value;

    return;
}

Index UniformGrid::getNumVertices() const {
    return getNumDivisions(0)*getNumDivisions(1)*getNumDivisions(2);
}

// GET BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector, Vector> UniformGrid::getBounds() const {

    return std::make_pair(Vector(m_min[0], m_min[1], m_min[2]), Vector(m_max[0], m_max[1], m_max[2]));
}

// CELL BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector, Vector> UniformGrid::cellBounds(Index elem) const {

    auto n = cellCoordinates(elem, m_numDivisions);
    Vector min(m_min[0]+n[0]*m_dist[0], m_min[1]+n[1]*m_dist[1], m_min[2]+n[2]*m_dist[2]);
    Vector max = min + Vector(m_dist[0], m_dist[1], m_dist[2]);
    return std::make_pair(min, max);
}

// FIND CELL
//-------------------------------------------------------------------------
Index UniformGrid::findCell(const Vector &point, bool acceptGhost) const {

    for (int c=0; c<3; ++c) {
        if (point[c] < m_min[c])
            return InvalidIndex;
        if (point[c] > m_max[c])
            return InvalidIndex;
    }

    Index n[3];
    for (int c=0; c<3; ++c) {
        n[c] = (point[c]-m_min[c])/m_dist[c];
        if (n[c] >= m_numDivisions[c]-1)
            n[c] = m_numDivisions[c]-2;
    }

    Index el = cellIndex(n, m_numDivisions);
    assert(inside(el, point));
    if (acceptGhost || !isGhostCell(el))
        return el;
    return InvalidIndex;
}

// INSIDE CHECK
//-------------------------------------------------------------------------
bool UniformGrid::inside(Index elem, const Vector &point) const {

    for (int c=0; c<3; ++c) {
        if (point[c] < m_min[c])
            return false;
        if (point[c] > m_max[c])
            return false;
    }

    std::array<Index,3> n = cellCoordinates(elem, m_numDivisions);
    for (int c=0; c<3; ++c) {
        Scalar x = m_min[c]+n[c]*m_dist[c];
        if (point[c] < x)
            return false;
        if (point[c] > x+m_dist[c])
            return false;
    }

    return true;
}

// GET INTERPOLATOR
//-------------------------------------------------------------------------
GridInterface::Interpolator UniformGrid::getInterpolator(Index elem, const Vector &point, DataBase::Mapping mapping, GridInterface::InterpolationMode mode) const {

   vassert(inside(elem, point));

#ifdef INTERPOL_DEBUG
   if (!inside(elem, point)) {
      return Interpolator();
   }
#endif

   if (mapping == DataBase::Element) {
       std::vector<Scalar> weights(1, 1.);
       std::vector<Index> indices(1, elem);

       return Interpolator(weights, indices);
   }

   std::array<Index,3> n = cellCoordinates(elem, m_numDivisions);
   std::array<Index,8> cl = cellVertices(elem, m_numDivisions);

   Vector corner(m_min[0]+n[0]*m_dist[0],
               m_min[1]+n[1]*m_dist[1],
               m_min[2]+n[2]*m_dist[2]);
   const Vector diff = point-corner;

   const Index nvert = 8;
   std::vector<Index> indices((mode==Linear || mode==Mean) ? nvert : 1);
   std::vector<Scalar> weights((mode==Linear || mode==Mean) ? nvert : 1);

   if (mode == Mean) {
       const Scalar w = Scalar(1)/nvert;
       for (Index i=0; i<nvert; ++i) {
           indices[i] = cl[i];
           weights[i] = w;
       }
   } else if (mode == Linear) {
       vassert(nvert == 8);
       for (Index i=0; i<nvert; ++i) {
           indices[i] = cl[i];
       }
       Vector ss = diff;
       for (int c=0; c<3; ++c) {
           ss[c] /= m_dist[c];
           assert(ss[c] >= 0.0);
           assert(ss[c] <= 1.0);
       }
       weights[0] = (1-ss[0])*(1-ss[1])*(1-ss[2]);
       weights[1] = ss[0]*(1-ss[1])*(1-ss[2]);
       weights[2] = ss[0]*ss[1]*(1-ss[2]);
       weights[3] = (1-ss[0])*ss[1]*(1-ss[2]);
       weights[4] = (1-ss[0])*(1-ss[1])*ss[2];
       weights[5] = ss[0]*(1-ss[1])*ss[2];
       weights[6] = ss[0]*ss[1]*ss[2];
       weights[7] = (1-ss[0])*ss[1]*ss[2];
   } else {
      weights[0] = 1;

      if (mode == First) {
         indices[0] = cl[0];
      } else if(mode == Nearest) {
          Vector ss = diff;
          int nearest=0;
          for (int c=0; c<3; ++c) {
              nearest <<= 1;
              ss[c] /= m_dist[c];
              if (ss[c] < 0.5)
                  nearest |= 1;
          }
          indices[0] = cl[nearest];
      }
   }

   return Interpolator(weights, indices);
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

    for (int i=0; i<3; ++i) {
        ghostLayers[i].construct(2);
    }
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

        ghostLayers[i].construct(2);
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
