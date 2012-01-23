#include <sstream>
#include <iomanip>

#include "object.h"
#include "tables.h"

#include "IsoSurface.h"

MODULE_MAIN(IsoSurface)


IsoSurface::IsoSurface(int rank, int size, int moduleID)
   : Module("IsoSurface", rank, size, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");
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

vistle::Triangles *
IsoSurface::generateIsoSurface(const vistle::UnstructuredGrid * grid,
                               const vistle::Vec<float> * data,
                               const float isoValue) {

   size_t numElem = grid->getNumElements();
   vistle::Triangles *t = vistle::Triangles::create();

   size_t numVertices = 0;

   for (size_t elem = 0; elem < numElem; elem ++) {

      switch ((*grid->tl)[elem]) {

         case vistle::UnstructuredGrid::HEXAHEDRON: {

            float3 vertlist[12];
            float3 v[8];
            size_t index[8];
            float field[8];
            size_t p = (*grid->el)[elem];

            index[0] = (*grid->cl)[p + 5];
            index[1] = (*grid->cl)[p + 6];
            index[2] = (*grid->cl)[p + 2];
            index[3] = (*grid->cl)[p + 1];
            index[4] = (*grid->cl)[p + 4];
            index[5] = (*grid->cl)[p + 7];
            index[6] = (*grid->cl)[p + 3];
            index[7] = (*grid->cl)[p];

            uint tableIndex = 0;
            for (int idx = 0; idx < 8; idx ++) {
               v[idx].x = (*grid->x)[index[idx]];
               v[idx].y = (*grid->y)[index[idx]];
               v[idx].z = (*grid->z)[index[idx]];
               field[idx] = (*data->x)[index[idx]];
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
            break;
         }

         default:
            break;
      }
   }

   return t;
}


bool IsoSurface::compute() {

   std::list<vistle::Object *> gridObjects = getObjects("grid_in");
   std::cout << "IsoSurface: " << gridObjects.size() << " grid objects"
             << std::endl;

   std::list<vistle::Object *> dataObjects = getObjects("data_in");
   std::cout << "IsoSurface: " << dataObjects.size() << " data objects"
             << std::endl;

   while (gridObjects.size() > 0 && dataObjects.size() > 0) {

      if (gridObjects.front()->getType() == vistle::Object::UNSTRUCTUREDGRID &&
          dataObjects.front()->getType() == vistle::Object::VECFLOAT) {

         vistle::UnstructuredGrid *usg =
            static_cast<vistle::UnstructuredGrid *>(gridObjects.front());

         vistle::Vec<float> *array =
            static_cast<vistle::Vec<float> *>(dataObjects.front());

         vistle::Triangles *triangles = generateIsoSurface(usg, array, 1.0);
         if (triangles)
            addObject("grid_out", triangles);
      }

      removeObject("grid_in", gridObjects.front());
      removeObject("data_in", dataObjects.front());

      gridObjects = getObjects("grid_in");
      dataObjects = getObjects("data_in");
   }

   return true;
}
