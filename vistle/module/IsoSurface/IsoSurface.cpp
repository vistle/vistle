#include <sstream>
#include <iomanip>

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

   setDefaultCacheMode(ObjectCache::CacheAll);

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");

   addFloatParameter("isovalue", "isovalue", 0.0);
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

class Leveller {

   UnstructuredGrid::const_ptr m_grid;
   std::vector<Object::const_ptr> m_data;
   Scalar m_isoValue;

   unsigned char *tl = NULL;
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

   Triangles::ptr m_triangles;
   Vec<Scalar>::ptr m_outData;

 public:
   Leveller(UnstructuredGrid::const_ptr grid, const Scalar isovalue)
   : m_grid(grid)
   , m_isoValue(isovalue)
   {

      tl = &grid->tl()[0];
      el = &grid->el()[0];
      cl = &grid->cl()[0];
      x = &grid->x()[0];
      y = &grid->y()[0];
      z = &grid->z()[0];

      m_triangles = Triangles::ptr(new Triangles(Object::Initialized));
      m_triangles->setMeta(grid->meta());
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

      Index curidx = 0;
      std::vector<Index> outputIdx(numElem);
#pragma omp parallel for schedule (dynamic)
      for (Index elem=0; elem<numElem; ++elem) {

         Index n = 0;
         switch (tl[elem]) {
            case UnstructuredGrid::HEXAHEDRON: {
               n = processHexahedron(elem, 0, true /* count only */);
            }
         }
#pragma omp critical
         {
            outputIdx[elem] = curidx;
            curidx += n;
         }
      }

      //std::cerr << "CuttingSurface: " << curidx << " vertices" << std::endl;

      m_triangles->cl().resize(curidx);
      out_cl = m_triangles->cl().data();
      m_triangles->x().resize(curidx);
      out_x = m_triangles->x().data();
      m_triangles->y().resize(curidx);
      out_y = m_triangles->y().data();
      m_triangles->z().resize(curidx);
      out_z = m_triangles->z().data();

      if (m_outData) {
         m_outData->x().resize(curidx);
         out_d = m_outData->x().data();
      }

#pragma omp parallel for schedule (dynamic)
      for (Index elem = 0; elem<numElem; ++elem) {
         switch (tl[elem]) {
            case UnstructuredGrid::HEXAHEDRON: {
               processHexahedron(elem, outputIdx[elem], false);
            }
         }
      }

      //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

      return true;
   }

   Object::ptr result() {

      return m_triangles;
   }

 private:
   Index processHexahedron(const Index elem, const Index outIdx, bool numVertsOnly) {

      Index p = el[elem];

      Index index[8];
      index[0] = cl[p + 5];
      index[1] = cl[p + 6];
      index[2] = cl[p + 2];
      index[3] = cl[p + 1];
      index[4] = cl[p + 4];
      index[5] = cl[p + 7];
      index[6] = cl[p + 3];
      index[7] = cl[p];

      Scalar field[8];
      Scalar3 v[8];
      for (int idx = 0; idx < 8; idx ++) {
         v[idx].x = x[index[idx]];
         v[idx].y = y[index[idx]];
         v[idx].z = z[index[idx]];
         field[idx] = d[index[idx]];
      }

      uint tableIndex = 0;
      for (int idx = 0; idx < 8; idx ++)
         tableIndex += (((int) (field[idx] < m_isoValue)) << idx);

      int numVerts = hexaNumVertsTable[tableIndex];

      if (numVertsOnly) {
         return numVerts;
      }

      if (numVerts) {

         Scalar3 vertlist[12];
         vertlist[0] = interp(m_isoValue, v[0], v[1], field[0], field[1]);
         vertlist[1] = interp(m_isoValue, v[1], v[2], field[1], field[2]);
         vertlist[2] = interp(m_isoValue, v[2], v[3], field[2], field[3]);
         vertlist[3] = interp(m_isoValue, v[3], v[0], field[3], field[0]);

         vertlist[4] = interp(m_isoValue, v[4], v[5], field[4], field[5]);
         vertlist[5] = interp(m_isoValue, v[5], v[6], field[5], field[6]);
         vertlist[6] = interp(m_isoValue, v[6], v[7], field[6], field[7]);
         vertlist[7] = interp(m_isoValue, v[7], v[4], field[7], field[4]);

         vertlist[8] = interp(m_isoValue, v[0], v[4], field[0], field[4]);
         vertlist[9] = interp(m_isoValue, v[1], v[5], field[1], field[5]);
         vertlist[10] = interp(m_isoValue, v[2], v[6], field[2], field[6]);
         vertlist[11] = interp(m_isoValue, v[3], v[7], field[3], field[7]);

         for (int idx = 0; idx < numVerts; idx += 3) {

            int edge[3];
            Scalar3 *v[3];

            for (int i=0; i<3; ++i) {
               edge[i] = hexaTriTable[tableIndex][idx+i];
               v[i] = &vertlist[edge[i]];
            }

            for (int i=0; i<3; ++i) {
               out_cl[outIdx+idx+i] = outIdx+idx+i;
               out_x[outIdx+idx+i] = v[i]->x;
               out_y[outIdx+idx+i] = v[i]->y;
               out_z[outIdx+idx+i] = v[i]->z;
            }
         }
      }

      return numVerts;
   }
};


Object::ptr
IsoSurface::generateIsoSurface(Object::const_ptr grid_object,
                               Object::const_ptr data_object,
                               const Scalar isoValue) {

   if (!grid_object || !data_object)
      return Object::ptr();

   UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(grid_object);
   Vec<Scalar>::const_ptr data = Vec<Scalar>::as(data_object);

   if (!grid || !data) {
      std::cerr << "IsoSurface: incompatible input" << std::endl;
      return Object::ptr();
   }

   Leveller l(grid, isoValue);
   l.addData(data);
   l.process();
   return l.result();
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
