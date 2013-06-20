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

   size_t *el = NULL;
   size_t *cl = NULL;
   const Scalar *x = NULL;
   const Scalar *y = NULL;
   const Scalar *z = NULL;

   const Scalar *d = NULL;

   size_t *out_cl = NULL;
   Scalar *out_x = NULL;
   Scalar *out_y = NULL;
   Scalar *out_z = NULL;
   Scalar *out_d = NULL;

   // mapping between vertex indices in the incoming object and
   // vertex indices in the outgoing object
   std::vector<size_t> vertexMap;

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
      const size_t numElem = m_grid->getNumElements();

      size_t curidx = 0;
      std::vector<size_t> outputIdx(numElem);
#pragma omp parallel for schedule (dynamic)
      for (size_t elem=0; elem<numElem; ++elem) {

         size_t n = processPolygon(elem, 0, true /* count only */);
#pragma omp critical
         {
            outputIdx[elem] = curidx;
            curidx += n;
         }
      }

      //std::cerr << "CuttingSurface: " << curidx << " vertices" << std::endl;

      m_outGrid->cl().resize(curidx);
      out_cl = m_outGrid->cl().data();
      m_outGrid->x().resize(curidx);
      out_x = m_outGrid->x().data();
      m_outGrid->y().resize(curidx);
      out_y = m_outGrid->y().data();
      m_outGrid->z().resize(curidx);
      out_z = m_outGrid->z().data();

      if (m_outData) {
         m_outData->x().resize(curidx);
         out_d = m_outData->x().data();
      }

#pragma omp parallel for schedule (dynamic)
      for (size_t elem = 0; elem<numElem; ++elem) {
         processPolygon(elem, outputIdx[elem], false);
      }

      //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

      return true;
   }

   Object::ptr result() {

      return m_outGrid;
   }

 private:
   size_t processPolygon(const size_t element, const size_t outIdx, bool numVertsOnly) {

      Polygons::const_ptr &in = m_grid;
      Polygons::ptr &out = m_outGrid;


      size_t start = el[element];
      size_t end;
      if (element != in->getNumElements() - 1)
         end = el[element + 1] - 1;
      else
         end = in->getNumCorners() - 1;

      size_t numIn = 0;
      size_t numIsect = 0;

      for (size_t corner = start; corner <= end; corner ++) {
         Vector p(x[cl[corner]],
               y[cl[corner]],
               z[cl[corner]]);
         if ((p - m_point) * m_normal < 0)
            numIn ++;
      }

      if (numVertsOnly) {
         return numIn;
      }

      if (numIn == (end - start + 1)) {

         // if all vertices in the element are on the right side
         // of the cutting plane, insert the element and all vertices
         out->el().push_back(out->cl().size());

         for (size_t corner = start; corner <= end; corner ++) {

            size_t vertexID = cl[corner];
            size_t outID;

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

         for (size_t corner = start; corner <= end; corner ++) {

            size_t vertexID = cl[corner];

            Vector p(x[cl[corner]],
                  y[cl[corner]],
                  z[cl[corner]]);

            size_t former = (corner == start) ? end : corner - 1;
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

               size_t outID = out->x().size();
               out->x().push_back(pp.x);
               out->y().push_back(pp.y);
               out->z().push_back(pp.z);

               out->cl().push_back(outID);
            }

            if ((p - m_point) * m_normal < 0) {

               size_t outID;

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

      return numIn;
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

         const size_t *el = &in->el()[0];
         const size_t *cl = &in->cl()[0];
         const Scalar *x = &in->x()[0];
         const Scalar *y = &in->y()[0];
         const Scalar *z = &in->z()[0];

         size_t numElements = in->getNumElements();
         for (size_t element = 0; element < numElements; element ++) {

            size_t start = el[element];
            size_t end;
            if (element != in->getNumElements() - 1)
               end = el[element + 1] - 1;
            else
               end = in->getNumCorners() - 1;

            size_t numIn = 0;

            for (size_t corner = start; corner <= end; corner ++) {
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

               for (size_t corner = start; corner <= end; corner ++) {

                  size_t vertexID = cl[corner];
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

               for (size_t corner = start; corner <= end; corner ++) {

                  size_t vertexID = cl[corner];

                  Vector p(x[cl[corner]],
                        y[cl[corner]],
                        z[cl[corner]]);

                  size_t former = (corner == start) ? end : corner - 1;
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

                     size_t outID = out->x().size();
                     out->x().push_back(pp.x);
                     out->y().push_back(pp.y);
                     out->z().push_back(pp.z);

                     out->cl().push_back(outID);
                  }

                  if ((p - point) * normal < 0) {

                     size_t outID;

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

