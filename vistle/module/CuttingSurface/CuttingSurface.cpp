#include <sstream>
#include <iomanip>

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

   setDefaultCacheMode(ObjectCache::CacheAll);

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");
   createOutputPort("data_out");

   addVectorParameter("point", "point on plane", ParamVector(0.0, 0.0, 0.0));
   addVectorParameter("vertex", "normal on plane", ParamVector(0.0, 0.0, 1.0));
}

CuttingSurface::~CuttingSurface() {

}

#define lerp(a, b, t) ( a + t * (b - a) )

typedef struct {
   Scalar x, y, z;
} Scalar3;

const Scalar EPSILON = 1.0e-10f;

inline Scalar3 lerp3(const Scalar3 & a, const Scalar3 & b, const Scalar t) {

   Scalar3 res;
   res.x = lerp(a.x, b.x, t);
   res.y = lerp(a.y, b.y, t);
   res.z = lerp(a.z, b.z, t);
   return res;
}

inline Scalar3 interp(Scalar value, const Scalar3 & p0, const Scalar3 & p1,
                     const Scalar f0, const Scalar f1,
                     const Scalar v0, const Scalar v1, Scalar & v) {

   Scalar diff = (f1 - f0);

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

   Scalar t = (value - f0) / diff;
   v = v0 + t * (v1 - v0);

   return lerp3(p0, p1, t);
}


class PlaneCut {

   UnstructuredGrid::const_ptr m_grid;
   std::vector<Object::const_ptr> m_data;
   Vector m_normal;
   Scalar m_distance;

   unsigned char *tl = NULL;
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

   Triangles::ptr m_triangles;
   Vec<Scalar>::ptr m_outData;

   bool m_countOnly = true;

 public:
   PlaneCut(UnstructuredGrid::const_ptr grid, const Vector &normal, const Scalar distance)
   : m_grid(grid)
   , m_normal(normal)
   , m_distance(distance) {

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
      const size_t numElem = m_grid->getNumElements();

      size_t curidx = 0;
      std::vector<size_t> outputIdx(numElem);
#pragma omp parallel for schedule (dynamic)
      for (size_t elem=0; elem<numElem; ++elem) {

         size_t n = 0;
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
      for (size_t elem = 0; elem<numElem; ++elem) {
         switch (tl[elem]) {
            case UnstructuredGrid::HEXAHEDRON: {
               processHexahedron(elem, outputIdx[elem], false);
            }
         }
      }

      //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

      return true;
   }

   std::pair<Object::ptr, Object::ptr> result() {

      return std::make_pair(m_triangles, m_outData);
   }

 private:
   size_t processHexahedron(const size_t elem, const size_t outIdx, bool numVertsOnly) {

      const size_t p = el[elem];
      size_t index[8];
      index[0] = cl[p + 5];
      index[1] = cl[p + 6];
      index[2] = cl[p + 2];
      index[3] = cl[p + 1];
      index[4] = cl[p + 4];
      index[5] = cl[p + 7];
      index[6] = cl[p + 3];
      index[7] = cl[p];

      Scalar3 v[8];
      Scalar field[8];
      for (int idx = 0; idx < 8; idx ++) {
         v[idx].x = x[index[idx]];
         v[idx].y = y[index[idx]];
         v[idx].z = z[index[idx]];
         field[idx] = (m_normal * Vector(v[idx].x, v[idx].y, v[idx].z) - m_distance);
      }

      uint tableIndex = 0;
      for (int idx = 0; idx < 8; idx ++)
         tableIndex += (((int) (field[idx] < 0.0)) << idx);

      int numVerts = hexaNumVertsTable[tableIndex];

      if (numVertsOnly) {
         return numVerts;
      }

      if (numVerts) {
         Scalar mapping[8];
         if (d) {
            for (int idx = 0; idx < 8; idx ++)
               mapping[idx] = d[index[idx]];
         }

         Scalar3 vertlist[12];
         Scalar maplist[12];

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
            for (int i=0; i<3; ++i) {
               edge[i] = hexaTriTable[tableIndex][idx+i];
               v[i] = &vertlist[edge[i]];
            }

            for (int i=0; i<3; ++i) {
               out_cl[outIdx+idx+i] = outIdx+idx+i;
               out_x[outIdx+idx+i] = v[i]->x;
               out_y[outIdx+idx+i] = v[i]->y;
               out_z[outIdx+idx+i] = v[i]->z;

               out_d[outIdx+idx+i] = maplist[edge[i]];
            }
         }
      }

      return numVerts;
   }
};

std::pair<Object::ptr, Object::ptr>
CuttingSurface::generateCuttingSurface(Object::const_ptr grid_object,
                                       Object::const_ptr data_object,
                                       const Vector &normal,
                                       const Scalar distance) {

   if (!grid_object || !data_object)
      return std::make_pair(Object::ptr(), Object::ptr());

   UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(grid_object);
   Vec<Scalar>::const_ptr data = Vec<Scalar>::as(data_object);

   if (!grid || !data) {
      std::cerr << "CuttingSurface: incompatible input" << std::endl;
      return std::make_pair(Object::ptr(), Object::ptr());
   }

   PlaneCut cutter(grid, normal, distance);
   cutter.addData(data);
   cutter.process();
   return cutter.result();
}

bool CuttingSurface::compute() {

   const ParamVector pnormal = getVectorParameter("vertex");
   Vector normal(pnormal[0], pnormal[1], pnormal[2]);
   normal = normal * (1./normal.length());
   const ParamVector ppoint = getVectorParameter("point");
   const Vector point = Vector(ppoint[0], ppoint[1], ppoint[2]);
   const Vector proj = normal * (point*normal);
   int max = 0;
   if (std::abs(normal.y) > std::abs(normal.x)) {
      max = 1;
      if (std::abs(normal.z) > std::abs(normal.y))
         max = 2;
   } else if (std::abs(normal.z) > std::abs(normal.x)) {
      max = 2;
   }
   const Scalar distance = proj[max]/normal[max];

   while (hasObject("grid_in") && hasObject("data_in")) {

      Object::const_ptr grid = takeFirstObject("grid_in");
      Object::const_ptr data = takeFirstObject("data_in");

      std::pair<Object::ptr, Object::ptr> object =
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
