#include <sstream>
#include <iomanip>

#include "object.h"
#include "set.h"
#include "polygons.h"

#include "CutGeometry.h"

MODULE_MAIN(CutGeometry)

CutGeometry::CutGeometry(const std::string &shmname, int rank, int size, int moduleID)
   : Module("CutGeometry", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

CutGeometry::~CutGeometry() {

}

vistle::Object::ptr CutGeometry::cutGeometry(vistle::Object::const_ptr object,
                                          const vistle::Vector & point,
                                          const vistle::Vector & normal)
   const {

   if (object)
      switch (object->getType()) {

         case vistle::Object::SET: {
            vistle::Set::const_ptr in = boost::static_pointer_cast<const vistle::Set>(object);
            vistle::Set::ptr out(new vistle::Set);
            out->setBlock(in->getBlock());
            out->setTimestep(in->getTimestep());

            for (size_t index = 0; index < in->getNumElements(); index ++) {

               vistle::Object::ptr o = cutGeometry(in->getElement(index),
                                               point, normal);
               if (o)
                  out->addElement(o);
            }
            return out;
            break;
         }

         case vistle::Object::POLYGONS: {

            // mapping between vertex indices in the incoming object and
            // vertex indices in the outgoing object
            std::vector<int> vertexMap;

            vistle::Polygons::const_ptr in =
               boost::static_pointer_cast<const vistle::Polygons>(object);
            vistle::Polygons::ptr out(new vistle::Polygons);
            out->setBlock(in->getBlock());
            out->setTimestep(in->getTimestep());

            const size_t *el = &in->el()[0];
            const size_t *cl = &in->cl()[0];
            const vistle::Scalar *x = &in->x()[0];
            const vistle::Scalar *y = &in->y()[0];
            const vistle::Scalar *z = &in->z()[0];

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
                  vistle::Vector p(x[cl[corner]],
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

                     vistle::Vector p(x[cl[corner]],
                                            y[cl[corner]],
                                            z[cl[corner]]);

                     size_t former = (corner == start) ? end : corner - 1;
                     vistle::Vector pl(x[cl[former]],
                                             y[cl[former]],
                                             z[cl[former]]);

                     if (((p - point) * normal < 0 &&
                          (pl - point) * normal >= 0) ||
                         ((p - point) * normal >= 0 &&
                          (pl - point) * normal < 0)) {

                        vistle::Scalar s = (normal * (point - p)) /
                           (normal * (pl - p));
                        vistle::Vector pp = p + (pl - p) * s;

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
   return vistle::Object::ptr();
}

bool CutGeometry::compute() {

   vistle::Vector point(0.0, 0.0, 0.0);
   vistle::Vector normal(1.0, 0.0, 0.0);

   while(vistle::Object::const_ptr oin = takeFirstObject("grid_in")) {

      vistle::Object::ptr object = cutGeometry(oin, point, normal);
      if (object)
         addObject("grid_out", object);

   }
   return true;
}

