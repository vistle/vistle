#include <sstream>
#include <iomanip>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <core/object.h>
#include <core/triangles.h>
#include <core/unstr.h>
#include <core/vec.h>
#include "tables.h"

#include "CuttingSurface.h"

using namespace vistle;

MODULE_MAIN(CuttingSurface)

CuttingSurface::CuttingSurface(const std::string &shmname, int rank, int size, int moduleID)
   : Module("CuttingSurface", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");
   createOutputPort("data_out");

   addFloatParameter("distance", 0.0);
   addVectorParameter("normal", vistle::Vector(0.0, 0.0, 1.0));
#ifdef _OPENMP
   omp_set_num_threads(4);
#endif
}

CuttingSurface::~CuttingSurface() {

}

#define lerp(a, b, t) ( a + t * (b - a) )

typedef struct {
   vistle::Scalar x, y, z;
} Scalar3;

const vistle::Scalar EPSILON = 1.0e-10f;

inline Scalar3 lerp3(const Scalar3 & a, const Scalar3 & b, const vistle::Scalar t) {

   Scalar3 res;
   res.x = lerp(a.x, b.x, t);
   res.y = lerp(a.y, b.y, t);
   res.z = lerp(a.z, b.z, t);
   return res;
}

inline Scalar3 interp(vistle::Scalar value, const Scalar3 & p0, const Scalar3 & p1,
                     const vistle::Scalar f0, const vistle::Scalar f1,
                     const vistle::Scalar v0, const vistle::Scalar v1, vistle::Scalar & v) {

   vistle::Scalar diff = (f1 - f0);

   if (fabs(diff) < EPSILON) {
      v = v0;
      return p0;
   }

   if (fabs(value - f0) < EPSILON) {
      v = v1;
      return p0;
   }

   if (fabs(value - f1) < EPSILON) {
      v = v0;
      return p1;
   }

   vistle::Scalar t = (value - f0) / diff;
   v = v0 + t * (v1 - v0);

   return lerp3(p0, p1, t);
}

std::pair<vistle::Object::ptr, vistle::Object::ptr>
CuttingSurface::generateCuttingSurface(vistle::Object::const_ptr grid_object,
                                       vistle::Object::const_ptr data_object,
                                       const vistle::Vector &normal,
                                       const vistle::Scalar distance) {

   vistle::UnstructuredGrid::const_ptr grid;
   vistle::Vec<vistle::Scalar>::const_ptr data;

   if (!grid_object || !data_object)
      return std::make_pair((vistle::Object *) NULL, (vistle::Object *) NULL);

   if (grid_object->getType() == vistle::Object::UNSTRUCTUREDGRID &&
       data_object->getType() == vistle::Vec<Scalar>::type()) {
      grid = boost::static_pointer_cast<const vistle::UnstructuredGrid>(grid_object);
      data = boost::static_pointer_cast<const vistle::Vec<vistle::Scalar> >(data_object);
   }

   const unsigned char *tl = &grid->tl()[0];
   const size_t *el = &grid->el()[0];
   const size_t *cl = &grid->cl()[0];
   const vistle::Scalar *x = &grid->x()[0];
   const vistle::Scalar *y = &grid->y()[0];
   const vistle::Scalar *z = &grid->z()[0];

   const vistle::Scalar *d = &data->x()[0];

   size_t numElem = grid->getNumElements();
   vistle::Triangles *triangles = new vistle::Triangles(Object::Initialized);
   triangles->setMeta(grid_object->meta());

   vistle::Vec<vistle::Scalar> *outData = new vistle::Vec<vistle::Scalar>(Object::Initialized);
   outData->setMeta(data_object->meta());

   size_t numVertices = 0;

#pragma omp parallel for
   for (size_t elem = 0; elem < numElem; elem ++) {

      switch (tl[elem]) {

         case vistle::UnstructuredGrid::HEXAHEDRON: {

            Scalar3 vertlist[12];
            vistle::Scalar maplist[12];
            Scalar3 v[8];
            size_t index[8];
            vistle::Scalar field[8];
            vistle::Scalar mapping[8];
            size_t p = el[elem];

            index[0] = cl[p + 5];
            index[1] = cl[p + 6];
            index[2] = cl[p + 2];
            index[3] = cl[p + 1];
            index[4] = cl[p + 4];
            index[5] = cl[p + 7];
            index[6] = cl[p + 3];
            index[7] = cl[p];

            for (int idx = 0; idx < 8; idx ++)
               mapping[idx] = d[index[idx]];

            uint tableIndex = 0;
            for (int idx = 0; idx < 8; idx ++) {
               v[idx].x = x[index[idx]];
               v[idx].y = y[index[idx]];
               v[idx].z = z[index[idx]];
               field[idx] = (normal * vistle::Vector(v[idx].x, v[idx].y, v[idx].z) - distance);
            }

            for (int idx = 0; idx < 8; idx ++)
               tableIndex += (((int) (field[idx] < 0.0)) << idx);

            int numVerts = hexaNumVertsTable[tableIndex];
            if (numVerts) {
               numVertices += numVerts;
               vertlist[0] = interp(0.0, v[0], v[1], field[0], field[1],
                                    mapping[0], mapping[1], maplist[0]);
               vertlist[1] = interp(0.0, v[1], v[2], field[1], field[2],
                                    mapping[1], mapping[2], maplist[1]);
               vertlist[2] = interp(0.0, v[2], v[3], field[2], field[3],
                                    mapping[2], mapping[3], maplist[2]);
               vertlist[3] = interp(0.0, v[3], v[0], field[3], field[0],
                                    mapping[3], mapping[0], maplist[3]);

               vertlist[4] = interp(0.0, v[4], v[5], field[4], field[5],
                                    mapping[4], mapping[5], maplist[4]);
               vertlist[5] = interp(0.0, v[5], v[6], field[5], field[6],
                                    mapping[5], mapping[6], maplist[5]);
               vertlist[6] = interp(0.0, v[6], v[7], field[6], field[7],
                                    mapping[6], mapping[7], maplist[6]);
               vertlist[7] = interp(0.0, v[7], v[4], field[7], field[4],
                                    mapping[7], mapping[4], maplist[7]);

               vertlist[8] = interp(0.0, v[0], v[4], field[0], field[4],
                                    mapping[0], mapping[4], maplist[8]);
               vertlist[9] = interp(0.0, v[1], v[5], field[1], field[5],
                                    mapping[1], mapping[5], maplist[9]);
               vertlist[10] = interp(0.0, v[2], v[6], field[2], field[6],
                                     mapping[2], mapping[6], maplist[10]);
               vertlist[11] = interp(0.0, v[3], v[7], field[3], field[7],
                                     mapping[3], mapping[7], maplist[11]);

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
                     triangles->cl().push_back(triangles->x().size());
                     triangles->cl().push_back(triangles->x().size() + 1);
                     triangles->cl().push_back(triangles->x().size() + 2);

                     triangles->x().push_back(v[0]->x);
                     triangles->x().push_back(v[1]->x);
                     triangles->x().push_back(v[2]->x);

                     triangles->y().push_back(v[0]->y);
                     triangles->y().push_back(v[1]->y);
                     triangles->y().push_back(v[2]->y);

                     triangles->z().push_back(v[0]->z);
                     triangles->z().push_back(v[1]->z);
                     triangles->z().push_back(v[2]->z);

                     outData->x().push_back(maplist[edge[0]]);
                     outData->x().push_back(maplist[edge[1]]);
                     outData->x().push_back(maplist[edge[2]]);
                  }
               }
            }
            break;
         }

         default:
            break;
      }
   }

   return std::make_pair(triangles, outData);
}


bool CuttingSurface::compute() {

   const vistle::Scalar distance = getFloatParameter("distance");
   const vistle::Vector normal = getVectorParameter("normal");

   while (hasObject("grid_in") && hasObject("data_in")) {

      vistle::Object::const_ptr grid = takeFirstObject("grid_in");
      vistle::Object::const_ptr data = takeFirstObject("data_in");

      std::pair<vistle::Object::ptr, vistle::Object::ptr> object =
         generateCuttingSurface(grid, data,
                                normal, distance);

      if (object.first) {
         object.first->copyAttributes(grid);
         addObject("grid_out", object.first);
      }

      if (object.second) {
         object.second->copyAttributes(data);
         addObject("data_out", object.second);
      }
   }

   return true;
}
