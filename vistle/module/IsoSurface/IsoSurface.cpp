#include <sstream>
#include <iomanip>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <core/object.h>
#include <core/unstr.h>
#include <core/vec.h>
#include <core/triangles.h>

#include "tables.h"
#include "IsoSurface.h"

MODULE_MAIN(IsoSurface)

using namespace vistle;


IsoSurface::IsoSurface(const std::string &shmname, int rank, int size, int moduleID)
   : Module("IsoSurface", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");

   addFloatParameter("isovalue", 0.0);
#ifdef _OPENMP
   omp_set_num_threads(4);
#endif
}

IsoSurface::~IsoSurface() {

}

#define lerp(a, b, t) ( a + t * (b - a) )

typedef struct {
   Scalar x, y, z;
} Scalar3;

const Scalar EPSILON = 1.0e-10f;

inline Scalar3 lerp3(const Scalar3 &a, const Scalar3 &b, const Scalar t) {

   Scalar3 res;
   res.x = lerp(a.x, b.x, t);
   res.y = lerp(a.y, b.y, t);
   res.z = lerp(a.z, b.z, t);
   return res;
}

inline Scalar3 interp(Scalar iso, const Scalar3 &p0, const Scalar3 &p1, const Scalar &f0, const Scalar &f1) {

   Scalar diff = (f1 - f0);

   if (fabs(diff) < EPSILON)
      return p0;

   if (fabs(iso - f0) < EPSILON)
      return p0;

   if (fabs(iso - f1) < EPSILON)
      return p1;

   Scalar t = (iso - f0) / diff;

   return lerp3(p0, p1, t);
}

Object::ptr
IsoSurface::generateIsoSurface(Object::const_ptr grid_object,
                               Object::const_ptr data_object,
                               const Scalar isoValue) {

   if (!grid_object || !data_object)
      return Object::ptr();

   UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(grid_object);
   Vec<Scalar>::const_ptr data = Vec<Scalar>::as(data_object);


   const unsigned char *tl = &grid->tl()[0];
   const size_t *el = &grid->el()[0];
   const size_t *cl = &grid->cl()[0];
   const Scalar *x = &grid->x()[0];
   const Scalar *y = &grid->y()[0];
   const Scalar *z = &grid->z()[0];

   const Scalar *d = &data->x()[0];

   size_t numElem = grid->getNumElements();
   Triangles::ptr t(new Triangles(Object::Initialized));
   t->setMeta(grid_object->meta());

   size_t numVertices = 0;

#pragma omp parallel for
   for (size_t elem = 0; elem < numElem; elem ++) {

      switch (tl[elem]) {

         case UnstructuredGrid::HEXAHEDRON: {

            Scalar3 vertlist[12];
            Scalar3 v[8];
            size_t index[8];
            Scalar field[8];
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
                  Scalar3 *v[3];
                  edge[0] = hexaTriTable[tableIndex][idx];
                  v[0] = &vertlist[edge[0]];
                  edge[1] = hexaTriTable[tableIndex][idx + 1];
                  v[1] = &vertlist[edge[1]];
                  edge[2] = hexaTriTable[tableIndex][idx + 2];
                  v[2] = &vertlist[edge[2]];

#pragma omp critical
                  {
                     t->cl().push_back(t->x().size());
                     t->cl().push_back(t->x().size() + 1);
                     t->cl().push_back(t->x().size() + 2);

                     t->x().push_back(v[0]->x);
                     t->x().push_back(v[1]->x);
                     t->x().push_back(v[2]->x);

                     t->y().push_back(v[0]->y);
                     t->y().push_back(v[1]->y);
                     t->y().push_back(v[2]->y);

                     t->z().push_back(v[0]->z);
                     t->z().push_back(v[1]->z);
                     t->z().push_back(v[2]->z);
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

   const Scalar isoValue = getFloatParameter("isovalue");

   while (hasObject("grid_in") && hasObject("data_in")) {

      Object::const_ptr grid = takeFirstObject("grid_in");
      Object::const_ptr data = takeFirstObject("data_in");
      Object::ptr object =
         generateIsoSurface(grid, data, isoValue);

      if (object) {
         object->copyAttributes(data);
         object->copyAttributes(grid, false);
         addObject("grid_out", object);
      }
   }

   return true;
}
