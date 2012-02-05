#include <sstream>
#include <iomanip>

#include <omp.h>

#include "object.h"
#include "tables.h"

#include "IsoSurface.h"

MODULE_MAIN(IsoSurface)


IsoSurface::IsoSurface(int rank, int size, int moduleID)
   : Module("IsoSurface", rank, size, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");

   addFloatParameter("isovalue", 0.0);
   omp_set_num_threads(4);
}

IsoSurface::~IsoSurface() {

}

#define lerp(a, b, t) ( a + t * (b - a) )

typedef struct {
   float x, y, z;
} float3;

const float EPSILON = 1.0e-10f;

inline float3 lerp3(const float3 &a, const float3 &b, const float t) {

   float3 res;
   res.x = lerp(a.x, b.x, t);
   res.y = lerp(a.y, b.y, t);
   res.z = lerp(a.z, b.z, t);
   return res;
}

inline float3 interp(float iso, const float3 &p0, const float3 &p1, const float &f0, const float &f1) {

   float diff = (f1 - f0);

   if (fabs(diff) < EPSILON)
      return p0;

   if (fabs(iso - f0) < EPSILON)
      return p0;

   if (fabs(iso - f1) < EPSILON)
      return p1;

   float t = (iso - f0) / diff;

   return lerp3(p0, p1, t);
}

vistle::Object *
IsoSurface::generateIsoSurface(const vistle::Object * grid_object,
                               const vistle::Object * data_object,
                               const float isoValue) {

   const vistle::UnstructuredGrid *grid = NULL;
   const vistle::Vec<float> *data = NULL;

   if (!grid_object || !data_object)
      return NULL;

   if (grid_object->getType() == vistle::Object::SET &&
       data_object->getType() == vistle::Object::SET) {

      const vistle::Set *gset = static_cast<const vistle::Set *>(grid_object);
      const vistle::Set *dset = static_cast<const vistle::Set *>(data_object);
      if (gset->getNumElements() != dset->getNumElements())
         return NULL;

      vistle::Set *set = vistle::Set::create(gset->getNumElements());
      set->setBlock(gset->getBlock());
      set->setTimestep(gset->getTimestep());

      for (size_t index = 0; index < gset->getNumElements(); index ++)
         (*set->elements)[index] =
            generateIsoSurface(gset->getElement(index),
                               dset->getElement(index), isoValue);
      return set;
   }

   if (grid_object->getType() == vistle::Object::UNSTRUCTUREDGRID &&
       data_object->getType() == vistle::Object::VECFLOAT) {
      grid = static_cast<const vistle::UnstructuredGrid *>(grid_object);
      data = static_cast<const vistle::Vec<float> *>(data_object);
   }

   const char *tl = &((*grid->tl)[0]);
   const size_t *el = &((*grid->el)[0]);
   const size_t *cl = &((*grid->cl)[0]);
   const float *x = &((*grid->x)[0]);
   const float *y = &((*grid->y)[0]);
   const float *z = &((*grid->z)[0]);

   const float *d = &((*data->x)[0]);

   size_t numElem = grid->getNumElements();
   vistle::Triangles *t = vistle::Triangles::create();
   t->setBlock(grid_object->getBlock());
   t->setTimestep(grid_object->getTimestep());

   size_t numVertices = 0;

#pragma omp parallel for
   for (size_t elem = 0; elem < numElem; elem ++) {

      switch (tl[elem]) {

         case vistle::UnstructuredGrid::HEXAHEDRON: {

            float3 vertlist[12];
            float3 v[8];
            size_t index[8];
            float field[8];
            size_t p = el[elem];

            index[0] = cl[p + 5];
            index[1] = cl[p + 6];
            index[2] = cl[p + 2];
            index[3] = cl[p + 1];
            index[4] = cl[p + 4];
            index[5] = cl[p + 7];
            index[6] = cl[p + 3];
            index[7] = cl[p];

            uint tableIndex = 0;
            for (int idx = 0; idx < 8; idx ++) {
               v[idx].x = x[index[idx]];
               v[idx].y = y[index[idx]];
               v[idx].z = z[index[idx]];
               field[idx] = d[index[idx]];
            }

            for (int idx = 0; idx < 8; idx ++)
               tableIndex += (((int) (field[idx] < isoValue)) << idx);

            int numVerts = hexaNumVertsTable[tableIndex];
            if (numVerts) {
               numVertices += numVerts;
               vertlist[0] = interp(isoValue, v[0], v[1], field[0], field[1]);
               vertlist[1] = interp(isoValue, v[1], v[2], field[1], field[2]);
               vertlist[2] = interp(isoValue, v[2], v[3], field[2], field[3]);
               vertlist[3] = interp(isoValue, v[3], v[0], field[3], field[0]);

               vertlist[4] = interp(isoValue, v[4], v[5], field[4], field[5]);
               vertlist[5] = interp(isoValue, v[5], v[6], field[5], field[6]);
               vertlist[6] = interp(isoValue, v[6], v[7], field[6], field[7]);
               vertlist[7] = interp(isoValue, v[7], v[4], field[7], field[4]);

               vertlist[8] = interp(isoValue, v[0], v[4], field[0], field[4]);
               vertlist[9] = interp(isoValue, v[1], v[5], field[1], field[5]);
               vertlist[10] = interp(isoValue, v[2], v[6], field[2], field[6]);
               vertlist[11] = interp(isoValue, v[3], v[7], field[3], field[7]);

               for (int idx = 0; idx < numVerts; idx += 3) {

                  int edge[3];
                  float3 *v[3];
                  edge[0] = hexaTriTable[tableIndex][idx];
                  v[0] = &vertlist[edge[0]];
                  edge[1] = hexaTriTable[tableIndex][idx + 1];
                  v[1] = &vertlist[edge[1]];
                  edge[2] = hexaTriTable[tableIndex][idx + 2];
                  v[2] = &vertlist[edge[2]];

#pragma omp critical
                  {
                     t->cl->push_back(t->x->size());
                     t->cl->push_back(t->x->size() + 1);
                     t->cl->push_back(t->x->size() + 2);

                     t->x->push_back(v[0]->x);
                     t->x->push_back(v[1]->x);
                     t->x->push_back(v[2]->x);

                     t->y->push_back(v[0]->y);
                     t->y->push_back(v[1]->y);
                     t->y->push_back(v[2]->y);

                     t->z->push_back(v[0]->z);
                     t->z->push_back(v[1]->z);
                     t->z->push_back(v[2]->z);
                  }
               }
            }
            break;
         }

         default:
            break;
      }
   }

   return t;
}


bool IsoSurface::compute() {

   const float isoValue = getFloatParameter("isovalue");

   std::list<vistle::Object *> gridObjects = getObjects("grid_in");
   std::cout << "IsoSurface: " << gridObjects.size() << " grid objects"
             << std::endl;

   std::list<vistle::Object *> dataObjects = getObjects("data_in");
   std::cout << "IsoSurface: " << dataObjects.size() << " data objects"
             << std::endl;

   while (gridObjects.size() > 0 && dataObjects.size() > 0) {

      vistle::Object *object =
         generateIsoSurface(gridObjects.front(), dataObjects.front(), isoValue);

      if (object)
         addObject("grid_out", object);

      removeObject("grid_in", gridObjects.front());
      removeObject("data_in", dataObjects.front());

      gridObjects = getObjects("grid_in");
      dataObjects = getObjects("data_in");
   }

   return true;
}
