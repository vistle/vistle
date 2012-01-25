#include <sstream>
#include <iomanip>

#include "object.h"

#include "CutGeometry.h"

MODULE_MAIN(CutGeometry)

CutGeometry::CutGeometry(int rank, int size, int moduleID)
   : Module("CutGeometry", rank, size, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

CutGeometry::~CutGeometry() {

}

vistle::Object * CutGeometry::cutGeometry(const vistle::Object * object,
                                          const Vec3 & point,
                                          const Vec3 & normal) const {

   if (object)
      switch (object->getType()) {

         case vistle::Object::SET: {
            const vistle::Set *in = static_cast<const vistle::Set *>(object);
            vistle::Set *out = vistle::Set::create();
            for (size_t index = 0; index < in->getNumElements(); index ++) {

               vistle::Object *o = cutGeometry(in->getElement(index),
                                               point, normal);
               if (o)
                  out->elements->push_back(o);
            }
            return out;
            break;
         }

         case vistle::Object::POLYGONS: {

            std::map<int, int> vertexMap;

            const vistle::Polygons *in =
               static_cast<const vistle::Polygons *>(object);
            vistle::Polygons *out = vistle::Polygons::create();

            size_t numElements = in->getNumElements();
            for (size_t element = 0; element < numElements; element ++) {

               size_t start = (*in->el)[element];
               size_t end;
               if (element != in->getNumElements() - 1)
                  end = (*in->el)[element + 1] - 1;
               else
                  end = in->getNumCorners() - 1;

               size_t numIn = 0;

               for (size_t corner = start; corner <= end; corner ++) {
                  Vec3 p((*in->x)[(*in->cl)[corner]],
                         (*in->y)[(*in->cl)[corner]],
                         (*in->z)[(*in->cl)[corner]]);
                  if ((p - point) * normal < 0)
                     numIn ++;
               }

               if (numIn == (end - start + 1)) {

                  out->el->push_back(out->cl->size());

                  for (size_t corner = start; corner <= end; corner ++) {

                     int vertexID = (*in->cl)[corner];
                     int outID;

                     std::map<int, int>::iterator i =
                        vertexMap.find(vertexID);

                     if (i == vertexMap.end()) {
                        outID = out->x->size();
                        vertexMap[vertexID] = outID;
                        out->x->push_back((*in->x)[vertexID]);
                        out->y->push_back((*in->y)[vertexID]);
                        out->z->push_back((*in->z)[vertexID]);
                     } else
                        outID = i->second;

                     out->cl->push_back(outID);
                  }
               } else if (numIn > 0) {

                  out->el->push_back(out->cl->size());

                  for (size_t corner = start; corner <= end; corner ++) {

                     int vertexID = (*in->cl)[corner];

                     Vec3 p((*in->x)[(*in->cl)[corner]],
                            (*in->y)[(*in->cl)[corner]],
                            (*in->z)[(*in->cl)[corner]]);

                     size_t last = (corner == start) ? end : corner - 1;
                     Vec3 pl((*in->x)[(*in->cl)[last]],
                             (*in->y)[(*in->cl)[last]],
                             (*in->z)[(*in->cl)[last]]);

                     if (((p - point) * normal < 0 &&
                          (pl - point) * normal >= 0) ||
                         ((p - point) * normal >= 0 &&
                          (pl - point) * normal < 0)) {

                        // insert intersection point
                        float s = (normal * (point - p)) /
                           (normal * (pl - p));
                        Vec3 pp = p + (pl - p) * s;

                        size_t outID = out->x->size();
                        out->x->push_back(pp.x);
                        out->y->push_back(pp.y);
                        out->z->push_back(pp.z);

                        out->cl->push_back(outID);
                     }

                     if ((p - point) * normal < 0) {

                        size_t outID;

                        std::map<int, int>::iterator i =
                           vertexMap.find(vertexID);

                        if (i == vertexMap.end()) {
                           outID = out->x->size();
                           vertexMap[vertexID] = outID;
                           out->x->push_back((*in->x)[vertexID]);
                           out->y->push_back((*in->y)[vertexID]);
                           out->z->push_back((*in->z)[vertexID]);
                        } else
                           outID = i->second;

                        out->cl->push_back(outID);
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
   return NULL;
}

bool CutGeometry::compute() {

   std::list<vistle::Object *> objects = getObjects("grid_in");
   std::cout << "CutGeometry: " << objects.size() << " objects" << std::endl;

   Vec3 point(0.1, 0.0, 0.0);
   Vec3 normal(1.0, 0.0, 0.0);

   std::list<vistle::Object *>::iterator oit;
   for (oit = objects.begin(); oit != objects.end(); oit ++) {

      vistle::Object *object = cutGeometry(*oit, point, normal);
      if (object)
         addObject("grid_out", object);

      removeObject("grid_in", object);
   }
   return true;
}

