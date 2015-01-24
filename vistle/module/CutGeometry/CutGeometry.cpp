#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/polygons.h>
#include <core/triangles.h>

#include "CutGeometry.h"

MODULE_MAIN(CutGeometry)

using namespace vistle;

CutGeometry::CutGeometry(const std::string &shmname, const std::string &name, int moduleID)
   : Module("CutGeometry", shmname, name, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheAll);

   createInputPort("grid_in");
   createOutputPort("grid_out");

   addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
   addVectorParameter("vertex", "normal on plane", ParamVector(0.0, 0.0, 1.0));
}

CutGeometry::~CutGeometry() {

}

class PlaneClip {

   Coords::const_ptr m_coord;
   Triangles::const_ptr m_tri;
   Polygons::const_ptr m_poly;
   std::vector<Object::const_ptr> m_data;
   Vector m_point;
   Vector m_normal;

   Index *el;
   Index *cl;
   const Scalar *x;
   const Scalar *y;
   const Scalar *z;

   const Scalar *d;

   Index *out_cl;
   Index *out_el;
   Scalar *out_x;
   Scalar *out_y;
   Scalar *out_z;
   Scalar *out_d;

   // mapping from vertex indices in the incoming object to
   // vertex indices in the outgoing object
   // - 1 is added to the entry, 0 marks entries that are to be erased
   std::vector<Index> m_vertexMap;

   Coords::ptr m_outCoords;
   Triangles::ptr m_outTri;
   Polygons::ptr m_outPoly;
   Vec<Scalar>::ptr m_outData;

 public:
   PlaneClip(Triangles::const_ptr grid, const Vector &point, const Vector &normal)
      : m_coord(grid)
      , m_tri(grid)
      , m_point(point)
      , m_normal(normal)
      , cl(nullptr)
      , x(nullptr)
      , y(nullptr)
      , z(nullptr)
      , d(nullptr)
      , out_cl(nullptr)
      , out_el(nullptr)
      , out_x(nullptr)
      , out_y(nullptr)
      , out_z(nullptr)
      , out_d(nullptr)
      {

         cl = &grid->cl()[0];
         x = &grid->x()[0];
         y = &grid->y()[0];
         z = &grid->z()[0];

         m_outTri = Triangles::ptr(new Triangles(Object::Initialized));
         m_outTri->setMeta(grid->meta());
         m_outCoords = m_outTri;
      }

   PlaneClip(Polygons::const_ptr grid, const Vector &point, const Vector &normal)
      : m_coord(grid)
      , m_poly(grid)
      , m_point(point)
      , m_normal(normal)
      , el(nullptr)
      , cl(nullptr)
      , x(nullptr)
      , y(nullptr)
      , z(nullptr)
      , d(nullptr)
      , out_cl(nullptr)
      , out_el(nullptr)
      , out_x(nullptr)
      , out_y(nullptr)
      , out_z(nullptr)
      , out_d(nullptr)
      {

         el = &grid->el()[0];
         cl = &grid->cl()[0];
         x = &grid->x()[0];
         y = &grid->y()[0];
         z = &grid->z()[0];

         m_outPoly = Polygons::ptr(new Polygons(Object::Initialized));
         m_outPoly->setMeta(grid->meta());
         m_outCoords = m_outPoly;
      }

   void addData(Object::const_ptr obj) {

      m_data.push_back(obj);
      auto data = Vec<Scalar, 1>::as(obj);
      if (data) {
         d = &data->x()[0];

         m_outData = Vec<Scalar>::ptr(new Vec<Scalar>(Object::Initialized));
         m_outData->setMeta(data->meta());
      }
   }

   bool process() {

      processCoordinates();
      Index numCoordsIn = m_outCoords->getNumCoords();

      if (m_tri) {
         const Index numElem = m_tri->getNumCorners()/3;

         std::vector<Index> outIdxCorner(numElem+1), outIdxCoord(numElem+1);
         outIdxCorner[0] = 0;
         outIdxCoord[0] = numCoordsIn;
#pragma omp parallel for schedule (dynamic)
         for (Index elem=0; elem<numElem; ++elem) {

            Index numCorner=0, numCoord=0;
            processTriangle(elem, numCorner, numCoord, true);

            outIdxCorner[elem+1] = numCorner;
            outIdxCoord[elem+1] = numCoord;
         }

         for (Index elem=0; elem<numElem; ++elem) {
            outIdxCoord[elem+1] += outIdxCoord[elem];
            outIdxCorner[elem+1] += outIdxCorner[elem];
         }

         Index numCoord = outIdxCoord[numElem];
         Index numCorner = outIdxCorner[numElem];

         m_outTri->cl().resize(numCorner);
         out_cl = m_outTri->cl().data();

         m_outTri->setSize(numCoord);
         out_x = m_outTri->x().data();
         out_y = m_outTri->y().data();
         out_z = m_outTri->z().data();

         if (m_outData) {
            m_outData->x().resize(numCoord);
            out_d = m_outData->x().data();
         }

#pragma omp parallel for schedule (dynamic)
         for (Index elem = 0; elem<numElem; ++elem) {
            processTriangle(elem, outIdxCorner[elem], outIdxCoord[elem], false);
         }
         //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

         return true;
      } else if (m_poly) {
         const Index numElem = m_poly->getNumElements();

         std::vector<Index> outIdxPoly(numElem+1), outIdxCorner(numElem+1), outIdxCoord(numElem+1);
         outIdxPoly[0] = 0;
         outIdxCorner[0] = 0;
         outIdxCoord[0] = numCoordsIn;
#pragma omp parallel for schedule (dynamic)
         for (Index elem=0; elem<numElem; ++elem) {

            Index numPoly=0, numCorner=0, numCoord=0;
            processPolygon(elem, numPoly, numCorner, numCoord, true);
            outIdxPoly[elem+1] = numPoly;
            outIdxCorner[elem+1] = numCorner;
            outIdxCoord[elem+1] = numCoord;
         }

         for (Index elem=0; elem<numElem; ++elem) {
            outIdxPoly[elem+1] += outIdxPoly[elem];
            outIdxCorner[elem+1] += outIdxCorner[elem];
            outIdxCoord[elem+1] += outIdxCoord[elem];
         }
         Index numPoly = outIdxPoly[numElem];
         Index numCorner = outIdxCorner[numElem];
         Index numCoord = outIdxCoord[numElem];

         m_outPoly->el().resize(numPoly+1);
         out_el = m_outPoly->el().data();
         out_el[numPoly] = numCorner;
         m_outPoly->cl().resize(numCorner);
         out_cl = m_outPoly->cl().data();

         m_outPoly->setSize(numCoord);
         out_x = m_outPoly->x().data();
         out_y = m_outPoly->y().data();
         out_z = m_outPoly->z().data();

         if (m_outData) {
            m_outData->x().resize(numCoord);
            out_d = m_outData->x().data();
         }

#pragma omp parallel for schedule (dynamic)
         for (Index elem = 0; elem<numElem; ++elem) {
            processPolygon(elem, outIdxPoly[elem], outIdxCorner[elem], outIdxCoord[elem], false);
         }

         //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

         return true;
      }

      return true;
   }

   Object::ptr result() {

      return m_outCoords;
   }

 private:
   void processCoordinates() {

      const Index nCoord = m_coord->getNumCoords();
      Index numIn = 0;
      m_vertexMap.resize(nCoord);
      auto vertexMap = m_vertexMap.data();
#pragma omp parallel for
      for (Index i=0; i<nCoord; ++i) {
         const Vector p(x[i], y[i], z[i]);
         if ((p - m_point).dot(m_normal) < 0) {
#pragma omp critical
            {
               ++numIn;
               vertexMap[i] = numIn;
            }
         } else {
               vertexMap[i] = 0;
         }
      }

      m_outCoords->setSize(numIn);
      out_x = m_outCoords->x().data();
      out_y = m_outCoords->y().data();
      out_z = m_outCoords->z().data();
#pragma omp parallel for schedule(dynamic)
      for (Index i=0; i<nCoord; ++i) {
         Index idx = vertexMap[i];
         vassert(idx >= 0);
         if (idx > 0) {
            --idx;
            vassert(idx < numIn);
            out_x[idx] = x[i];
            out_y[idx] = y[i];
            out_z[idx] = z[i];
         }
      }
   }

   inline Vector splitEdge(Index i, Index j) {

      Vector p1(x[i], y[i], z[i]);
      Vector p2(x[j], y[j], z[j]);

      Vector dist = p2 - p1;
      Scalar s = m_normal.dot(m_point - p1) / m_normal.dot(dist);
      return  p1 + dist * s;
   }

   void processTriangle(const Index element, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly) {

      const Index start = element*3;
      const Index end = start+3;
      const Index nCorner = end - start;
      const auto vertexMap = m_vertexMap.data();

      Index numIn = 0;
      Index cornerIn = 0, cornerOut = 0; // for the case where we have to split edges, new triangles might be created
      // - make sure that all the vertices belonging to a triangle are emitted in order
      for (Index i=0; i<nCorner; ++i) {
         const Index corner = start+i;
         const Index vind = cl[corner];
         const bool in = vertexMap[vind] > 0;
         if (in) {
            cornerIn = i;
            ++numIn;
         } else {
            cornerOut = i;
         }
      }

      if (numIn == 0) {
         if (numVertsOnly) {
            outIdxCorner = 0;
            outIdxCoord = 0;
         }
         return;
      }

      if (numIn == nCorner) {
         // if all vertices in the element are on the right side
         // of the cutting plane, insert the element and all vertices

         if (numVertsOnly) {
            outIdxCorner = numIn;
            outIdxCoord = 0;
            return;
         }

         for (Index i=0; i<nCorner; ++i) {

            const Index corner = start+i;
            const Index vid = cl[corner];
            const Index outID = vertexMap[vid]-1;
            out_cl[outIdxCorner+i] = outID;
         }

      } else if (numIn > 0) {

         const Index totalCorner = numIn==1 ? 3 : 9;
         const Index newCoord = numIn==1 ? 2 : 3;

         if (numVertsOnly) {
            outIdxCorner = totalCorner;
            outIdxCoord = newCoord;
            return;
         }

         if (numIn == 1) {
            // in0 is the only pre-existing corner inside
            const Index in0 = cl[start+cornerIn];
            vassert(vertexMap[in0] > 0);
            const Index out0 = cl[start + (cornerIn+1)%nCorner];
            vassert(vertexMap[out0] == 0);
            const Vector v0 = splitEdge(in0, out0);
            out_x[outIdxCoord] = v0[0];
            out_y[outIdxCoord] = v0[1];
            out_z[outIdxCoord] = v0[2];

            const Index out1 = cl[start+(cornerIn+2)%nCorner];
            vassert(vertexMap[out1] == 0);
            const Vector v1 = splitEdge(in0, out1);
            out_x[outIdxCoord+1] = v1[0];
            out_y[outIdxCoord+1] = v1[1];
            out_z[outIdxCoord+1] = v1[2];

            Index n = 0;
            out_cl[outIdxCorner+n] = vertexMap[in0]-1;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord+1;
            ++n;
            vassert(n == totalCorner);

         } else if (numIn == 2) {
            const Index out0 = cl[start + cornerOut];
            const Index in0 = cl[start+(cornerOut+2)%nCorner];
            vassert(vertexMap[out0] == 0);
            const Vector v0 = splitEdge(in0, out0);
            out_x[outIdxCoord] = v0[0];
            out_y[outIdxCoord] = v0[1];
            out_z[outIdxCoord] = v0[2];

            const Index in1 = cl[start+(cornerOut+1)%nCorner];
            vassert(vertexMap[in1] > 0);
            const Vector v1 = splitEdge(in1, out0);
            out_x[outIdxCoord+1] = v1[0];
            out_y[outIdxCoord+1] = v1[1];
            out_z[outIdxCoord+1] = v1[2];

            const Vector vin0(x[in0], y[in0], z[in0]);
            const Vector vin1(x[in1], y[in1], z[in1]);
            const Vector v2 = (vin0+vin1)*Scalar(0.5);
            out_x[outIdxCoord+2] = v2[0];
            out_y[outIdxCoord+2] = v2[1];
            out_z[outIdxCoord+2] = v2[2];

            Index n = 0;
            out_cl[outIdxCorner+n] = vertexMap[in0]-1;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord+2;
            ++n;

            out_cl[outIdxCorner+n] = outIdxCoord+2;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord+1;
            ++n;

            out_cl[outIdxCorner+n] = outIdxCoord+2;
            ++n;
            out_cl[outIdxCorner+n] = outIdxCoord+1;
            ++n;
            out_cl[outIdxCorner+n] = vertexMap[in1]-1;
            ++n;
            vassert(n == totalCorner);
         }
      }
   }

   void processPolygon(const Index element, Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly) {

      const auto vertexMap = m_vertexMap.data();
      const Index start = el[element];
      Index end;
      if (element != m_poly->getNumElements() - 1)
         end = el[element + 1];
      else
         end = m_poly->getNumCorners();
      const Index nCorner = end - start;

      Index numIn = 0, numCreate = 0;
      if (nCorner > 0) {
         bool prevIn = vertexMap[cl[end-1]] > 0;
         for (Index corner = start; corner < end; ++corner) {
            const Index vind = cl[corner];
            const bool in = vertexMap[vind] > 0;
            if (in) {
               if (!prevIn) {
                  ++numCreate;
               }
               ++numIn;
               prevIn = true;
            } else {
               if (prevIn)
                  ++numCreate;
               prevIn = false;
            }
         }
      }

      if (numIn == 0) {
         if (numVertsOnly) {
            outIdxPoly = 0;
            outIdxCorner = 0;
            outIdxCoord = 0;
         }
         return;
      }

      if (numIn == nCorner) {
         // if all vertices in the element are on the right side
         // of the cutting plane, insert the element and all vertices

         if (numVertsOnly) {
            outIdxPoly = 1;
            outIdxCorner = numIn;
            outIdxCoord = 0;
            return;
         }

         out_el[outIdxPoly] = outIdxCorner;

         Index i = 0;
         for (Index corner = start; corner < end; ++corner) {

            const Index vid = cl[corner];
            const Index outID = vertexMap[vid]-1;
            out_cl[outIdxCorner+i] = outID;
            ++i;
         }

      } else if (numIn > 0) {
         // if not all of the vertices of an element are on the same
         // side of the cutting plane:
         //   - insert vertices that are on the right side of the plane
         //   - omit vertices that are on the wrong side of the plane
         //   - if the vertex before the processed vertex is on the
         //     other side of the plane: insert the intersection point
         //     between the line formed by the two vertices and the
         //     plane

         vassert(numCreate%2 == 0);
         vassert(numCreate == 2); // we only handle convex polygons
         if (numVertsOnly) {
            outIdxPoly = numCreate/2;
            outIdxPoly = 1;
            outIdxCorner = numIn + numCreate;
            outIdxCoord = numCreate;
            return;
         }

         out_el[outIdxPoly] = outIdxCorner;

         Index n = 0;
         Index prevIdx = cl[start + nCorner-1];
         Index numCreated = 0;
         bool prevIn = vertexMap[prevIdx] > 0;
         for (Index i = 0; i < nCorner; ++i) {

            const Index corner = start + i;
            const Index idx = cl[corner];
            Index outID = vertexMap[idx];
            const bool in = outID > 0;
            --outID;
            if (in != prevIn) {
               const Index newId = outIdxCoord + numCreated;
               ++numCreated;

               const Vector v = splitEdge(idx, prevIdx);
               out_x[newId] = v[0];
               out_y[newId] = v[1];
               out_z[newId] = v[2];
            
               out_cl[outIdxCorner+n] = newId;
               ++n;
            }
            if (in) {
               out_cl[outIdxCorner+n] = outID;
               ++n;
            }
            prevIdx = idx;
            prevIn = in;
         }
         vassert(numCreated == numCreate);
         vassert(n == numIn+numCreate);
      }
   }
};


Object::ptr CutGeometry::cutGeometry(Object::const_ptr object,
                                          const Vector & point,
                                          const Vector & normal) const {

   switch (object->getType()) {
      case Object::TRIANGLES: {

         PlaneClip cutter(Triangles::as(object), point, normal);
         cutter.process();
         return cutter.result();
      }

      case Object::POLYGONS: {

         PlaneClip cutter(Polygons::as(object), point, normal);
         cutter.process();
         return cutter.result();
      }

      default:
         break;
   }
   return Object::ptr();
}

bool CutGeometry::compute() {

   const ParamVector pnormal = getVectorParameter("vertex");
   Vector normal(pnormal[0], pnormal[1], pnormal[2]);
   normal.normalize();
   const ParamVector ppoint = getVectorParameter("point");
   Vector point(ppoint[0], ppoint[1], ppoint[2]);

   Object::const_ptr oin = expect<Object>("grid_in");
   if (!oin)
      return false;

   Object::ptr object = cutGeometry(oin, point, normal);
   if (object && !object->isEmpty()) {
      object->copyAttributes(oin);
      addObject("grid_out", object);
   }

   return true;
}

