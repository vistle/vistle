#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/polygons.h>

#include "CutGeometry.h"

MODULE_MAIN(CutGeometry)

using namespace vistle;

CutGeometry::CutGeometry(const std::string &shmname, int rank, int size, int moduleID)
   : Module("CutGeometry", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

CutGeometry::~CutGeometry() {

}

Object::ptr CutGeometry::cutGeometry(Object::const_ptr object,
                                          const Vector & point,
                                          const Vector & normal) const {

   if (!object)
      return Object::ptr();

   switch (object->getType()) {

      case Object::POLYGONS: {

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

   Vector point(0.0, 0.0, 0.0);
   Vector normal(1.0, 0.0, 0.0);

   while(Object::const_ptr oin = takeFirstObject("grid_in")) {

      Object::ptr object = cutGeometry(oin, point, normal);
      if (object) {
         object->copyAttributes(oin);
         addObject("grid_out", object);
      }

   }
   return true;
}

