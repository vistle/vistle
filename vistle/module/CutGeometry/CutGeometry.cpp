#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/polygons.h>

#include "CutGeometry.h"

MODULE_MAIN(CutGeometry)

using namespace vistle;

CutGeometry::CutGeometry(const std::string &shmname, int rank, int size, int moduleID)
   : Module("CutGeometry", shmname, rank, size, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheAll);

   createInputPort("grid_in");
   createOutputPort("grid_out");

   addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
   addVectorParameter("vertex", "normal on plane", ParamVector(0.0, 0.0, 1.0));
}

CutGeometry::~CutGeometry() {

}

class PlaneClip {

   Polygons::const_ptr m_grid;
   std::vector<Object::const_ptr> m_data;
   Vector m_point;
   Vector m_normal;

   Index *el = NULL;
   Index *cl = NULL;
   const Scalar *x = NULL;
   const Scalar *y = NULL;
   const Scalar *z = NULL;

   const Scalar *d = NULL;

   Index *out_cl = NULL;
   Scalar *out_x = NULL;
   Scalar *out_y = NULL;
   Scalar *out_z = NULL;
   Scalar *out_d = NULL;

   // mapping between vertex indices in the incoming object and
   // vertex indices in the outgoing object
   std::vector<Index> vertexMap;

   Polygons::ptr m_outGrid;
   Vec<Scalar>::ptr m_outData;

   bool m_countOnly = true;

 public:
   PlaneClip(Polygons::const_ptr grid, const Vector &point, const Vector &normal)
      : m_grid(grid)
      , m_point(point)
      , m_normal(normal) {

         el = &grid->el()[0];
         cl = &grid->cl()[0];
         x = &grid->x()[0];
         y = &grid->y()[0];
         z = &grid->z()[0];

         m_outGrid = Polygons::ptr(new Polygons(Object::Initialized));
         m_outGrid->setMeta(grid->meta());

         vertexMap.resize(m_grid->x().size());
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
      const Index numElem = m_grid->getNumElements();

      Index curPoly=0, curCorner=0, curCoord=0;
      std::vector<Index> outIdxPoly(numElem), outIdxCorner(numElem), outIdxCoord(numElem);
#pragma omp parallel for schedule (dynamic)
      for (Index elem=0; elem<numElem; ++elem) {

         Index numPoly=0, numCorner=0, numCoord=0;
         processPolygon(elem, numPoly, numCorner, numCoord, true);

#pragma omp critical
         {
            outIdxPoly[elem] = curPoly;
            outIdxCorner[elem] = curCorner;
            outIdxCoord[elem] = curCoord;
            curPoly += numPoly;
            curCorner += numCorner;
            curCoord += numCoord;
         }
      }

      //std::cerr << "CuttingSurface: " << curidx << " vertices" << std::endl;

      m_outGrid->cl().resize(curCorner);
      out_cl = m_outGrid->cl().data();
      m_outGrid->x().resize(curCoord);
      out_x = m_outGrid->x().data();
      m_outGrid->y().resize(curCoord);
      out_y = m_outGrid->y().data();
      m_outGrid->z().resize(curCoord);
      out_z = m_outGrid->z().data();

      if (m_outData) {
         m_outData->x().resize(curCoord);
         out_d = m_outData->x().data();
      }

#pragma omp parallel for schedule (dynamic)
      for (Index elem = 0; elem<numElem; ++elem) {
         processPolygon(elem, outIdxPoly[elem], outIdxCorner[elem], outIdxCoord[elem], false);
      }

      //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

      return true;
   }

   Object::ptr result() {

      return m_outGrid;
   }

 private:
   void processPolygon(const Index element, Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly) {

      Polygons::const_ptr &in = m_grid;
      Polygons::ptr &out = m_outGrid;


      Index start = el[element];
      Index end;
      if (element != in->getNumElements() - 1)
         end = el[element + 1] - 1;
      else
         end = in->getNumCorners() - 1;

      Index numIn = 0;
      Index numIsect = 0;

      for (Index corner = start; corner <= end; corner ++) {
         Vector p(x[cl[corner]],
               y[cl[corner]],
               z[cl[corner]]);
         if ((p - m_point) * m_normal < 0)
            numIn ++;
      }

      if (numVertsOnly) {
         return;
      }

      if (numIn == (end - start + 1)) {

         // if all vertices in the element are on the right side
         // of the cutting plane, insert the element and all vertices
         out->el().push_back(out->cl().size());

         for (Index corner = start; corner <= end; corner ++) {

            Index vertexID = cl[corner];
            Index outID;

            if (vertexMap[vertexID] <= 0) {
               outID = out->x().size();
               vertexMap[vertexID] = outID+1;
               out->x().push_back(x[vertexID]);
               out->y().push_back(y[vertexID]);
               out->z().push_back(z[vertexID]);
            } else {
               outID = vertexMap[vertexID]-1;
            }

            out->cl().push_back(outID);
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
         out->el().push_back(out->cl().size());

         for (Index corner = start; corner <= end; corner ++) {

            Index vertexID = cl[corner];

            Vector p(x[cl[corner]],
                  y[cl[corner]],
                  z[cl[corner]]);

            Index former = (corner == start) ? end : corner - 1;
            Vector pl(x[cl[former]],
                  y[cl[former]],
                  z[cl[former]]);

            if (((p - m_point) * m_normal < 0 &&
                     (pl - m_point) * m_normal >= 0) ||
                  ((p - m_point) * m_normal >= 0 &&
                   (pl - m_point) * m_normal < 0)) {

               Scalar s = (m_normal * (m_point - p)) /
                  (m_normal * (pl - p));
               Vector pp = p + (pl - p) * s;

               Index outID = out->x().size();
               out->x().push_back(pp.x);
               out->y().push_back(pp.y);
               out->z().push_back(pp.z);

               out->cl().push_back(outID);
            }

            if ((p - m_point) * m_normal < 0) {

               Index outID;

               if (vertexMap[vertexID] <= 0) {
                  outID = out->x().size();
                  vertexMap[vertexID] = outID+1;
                  out->x().push_back(x[vertexID]);
                  out->y().push_back(y[vertexID]);
                  out->z().push_back(z[vertexID]);
               } else {
                  outID = vertexMap[vertexID]-1;
               }

               out->cl().push_back(outID);
            }
         }
      }
   }
};


Object::ptr CutGeometry::cutGeometry(Object::const_ptr object,
                                          const Vector & point,
                                          const Vector & normal) const {

   if (!object)
      return Object::ptr();

   switch (object->getType()) {

      case Object::POLYGONS: {

         //PlaneCut cutter(Polygons::as(object), point, normal);

         // mapping between vertex indices in the incoming object and
         // vertex indices in the outgoing object
         std::vector<int> vertexMap;

         Polygons::const_ptr in = Polygons::as(object);
         Polygons::ptr out(new Polygons(Object::Initialized));

         const Index *el = &in->el()[0];
         const Index *cl = &in->cl()[0];
         const Scalar *x = &in->x()[0];
         const Scalar *y = &in->y()[0];
         const Scalar *z = &in->z()[0];

         Index numElements = in->getNumElements();
         for (Index element = 0; element < numElements; element ++) {

            Index start = el[element];
            Index end;
            if (element != in->getNumElements() - 1)
               end = el[element + 1] - 1;
            else
               end = in->getNumCorners() - 1;

            Index numIn = 0;

            for (Index corner = start; corner <= end; corner ++) {
               Vector p(x[cl[corner]],
                     y[cl[corner]],
                     z[cl[corner]]);
               if ((p - point) * normal < 0)
                  numIn ++;
            }

            if (numIn == (end - start + 1)) {

               // if all vertices in the element are on the right side
               // of the cutting plane, insert the element and all vertices
               out->el().push_back(out->cl().size());

               for (Index corner = start; corner <= end; corner ++) {

                  Index vertexID = cl[corner];
                  int outID;

                  if (vertexMap.size() < vertexID+1)
                     vertexMap.resize(vertexID+1);
                  if (vertexMap[vertexID] <= 0) {
                     outID = out->x().size();
                     vertexMap[vertexID] = outID+1;
                     out->x().push_back(x[vertexID]);
                     out->y().push_back(y[vertexID]);
                     out->z().push_back(z[vertexID]);
                  } else {
                     outID = vertexMap[vertexID]-1;
                  }

                  out->cl().push_back(outID);
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
               out->el().push_back(out->cl().size());

               for (Index corner = start; corner <= end; corner ++) {

                  Index vertexID = cl[corner];

                  Vector p(x[cl[corner]],
                        y[cl[corner]],
                        z[cl[corner]]);

                  Index former = (corner == start) ? end : corner - 1;
                  Vector pl(x[cl[former]],
                        y[cl[former]],
                        z[cl[former]]);

                  if (((p - point) * normal < 0 &&
                           (pl - point) * normal >= 0) ||
                        ((p - point) * normal >= 0 &&
                         (pl - point) * normal < 0)) {

                     Scalar s = (normal * (point - p)) /
                        (normal * (pl - p));
                     Vector pp = p + (pl - p) * s;

                     Index outID = out->x().size();
                     out->x().push_back(pp.x);
                     out->y().push_back(pp.y);
                     out->z().push_back(pp.z);

                     out->cl().push_back(outID);
                  }

                  if ((p - point) * normal < 0) {

                     Index outID;

                     if (vertexMap.size() < vertexID+1)
                        vertexMap.resize(vertexID+1);
                     if (vertexMap[vertexID] <= 0) {
                        outID = out->x().size();
                        vertexMap[vertexID] = outID+1;
                        out->x().push_back(x[vertexID]);
                        out->y().push_back(y[vertexID]);
                        out->z().push_back(z[vertexID]);
                     } else {
                        outID = vertexMap[vertexID]-1;
                     }

                     out->cl().push_back(outID);
                  }
               }
            }
         }
         return out;
         break;
      }

      default:
         break;
   }
   return Object::ptr();
}

bool CutGeometry::compute() {

   const ParamVector pnormal = getVectorParameter("vertex");
   Vector normal(pnormal[0], pnormal[1], pnormal[2]);
   normal = normal * (1./normal.length());
   const ParamVector ppoint = getVectorParameter("point");
   Vector point(ppoint[0], ppoint[1], ppoint[2]);

   while(Object::const_ptr oin = takeFirstObject("grid_in")) {

      Object::ptr object = cutGeometry(oin, point, normal);
      if (object) {
         object->copyAttributes(oin);
         addObject("grid_out", object);
      }

   }
   return true;
}

