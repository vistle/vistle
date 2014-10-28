//
//This code is used for both IsoCut and IsoSurface!
//

#include <sstream>
#include <iomanip>
#include <core/index.h>
#include <core/scalar.h>
#include <core/unstr.h>
#include <core/triangles.h>
#include <core/empty.h>
#include <core/shm.h>
#include <thrust/execution_policy.h>
#include <thrust/device_vector.h>
#include <thrust/transform.h>
#include <thrust/for_each.h>
#include <thrust/scan.h>
#include <thrust/sequence.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/tuple.h>
#include "tables.h"
#include "Leveller.h"

using namespace vistle;

#define lerp(a, b, t) ( a + t * (b - a) )

const Scalar EPSILON = 1.0e-10f;
const int maxNumdata = 6;

inline Scalar __host__ __device__ tinterp(Scalar iso, const Scalar &f0, const Scalar &f1) {

   Scalar diff = (f1 - f0);

   if (fabs(diff) < EPSILON)
      return 0;

   if (fabs(iso - f0) < EPSILON)
      return 0;

   if (fabs(iso - f1) < EPSILON)
      return 1;

   return (iso - f0) / diff;
}

#ifdef CUTTINGSURFACE

//! generate data on the fly for creating cutting surfaces as isosurfaces
struct IsoDataFunctor {

   IsoDataFunctor(const Vector &vertex, const Vector &point, const Vector &direction, const Scalar* x, const Scalar* y, const Scalar* z, int option)
      : m_x(x)
      , m_y(y)
      , m_z(z)
      , m_vertex(vertex)
      , m_point(point)
      , m_direction(direction)
      , m_distance(vertex.dot(point))
      , m_option(option)
      , m_vectorprod(m_direction.cross(m_point-m_vertex))
      , m_cylinderradius2(m_vectorprod.squaredNorm())
      , m_sphereradius2((m_vertex-m_point).squaredNorm())
   {}
   __host__ __device__ Scalar operator()(Index i) {

      switch(m_option) {
         case Plane: {
            Vector coordinates(m_x[i], m_y[i], m_z[i]);
            return m_vertex.dot(coordinates) - m_distance;
         }
         case Sphere: {
            Vector coordinates(m_x[i], m_y[i], m_z[i]);
            return coordinates.squaredNorm() - m_sphereradius2;
         }
         default: {
            // all cylinders
            Vector coordinates(m_x[i], m_y[i], m_z[i]);
            return (m_direction.cross(coordinates - m_vertex)).squaredNorm() - m_cylinderradius2;
         }
      }
   }
   const Scalar* m_x;
   const Scalar* m_y;
   const Scalar* m_z;
   const Vector m_vertex;
   const Vector m_point;
   const Vector m_direction;
   const Scalar m_distance;
   const int m_option;
   const Vector m_vectorprod;
   const Scalar m_cylinderradius2;
   const Scalar m_sphereradius2;
};

#else
//! fetch data from scalar field for generating isosurface
struct IsoDataFunctor {

   IsoDataFunctor(const Scalar *data)
      : m_volumedata(data)
   {}
   __host__ __device__ Scalar operator()(Index i) { return m_volumedata[i]; }

   const Scalar *m_volumedata;

};
#endif

struct HostData {

   Scalar m_isovalue;
   Index m_numinputdata;
   IsoDataFunctor m_isoFunc;
   const vistle::shm<Index>::array &m_el;
   const vistle::shm<Index>::array &m_cl;
   const vistle::shm<unsigned char>::array &m_tl;
   std::vector<Index> m_caseNums;
   std::vector<Index> m_numVertices;
   std::vector<Index> m_LocationList;
   std::vector<Index> m_ValidCellVector;
   const vistle::shm<Scalar>::array &m_x;
   const vistle::shm<Scalar>::array &m_y;
   const vistle::shm<Scalar>::array &m_z;
   std::vector<vistle::ShmVector<Scalar>::ptr> m_outData;
   std::vector<const Scalar*> m_inputpointer;
   std::vector<Scalar*> m_outputpointer;

   typedef vistle::shm<Index>::array::iterator IndexIterator;
   typedef std::vector<Index>::iterator VectorIndexIterator;

   HostData(Scalar isoValue
            , IsoDataFunctor isoFunc
            , const vistle::shm<Index>::array &el
            , const vistle::shm<unsigned char>::array &tl
            , const vistle::shm<Index>::array &cl
            , const vistle::shm<Scalar>::array &x
            , const vistle::shm<Scalar>::array &y
            , const vistle::shm<Scalar>::array &z
            )
      : m_isovalue(isoValue)
      , m_isoFunc(isoFunc)
      , m_el(el)
      , m_cl(cl)
      , m_tl(tl)
      , m_x(x)
      , m_y(y)
      , m_z(z)
   {
      m_inputpointer.push_back(x.data());
      m_inputpointer.push_back(y.data());
      m_inputpointer.push_back(z.data());

      for(size_t i = 0; i < m_inputpointer.size(); i++){
         m_outData.push_back(new vistle::ShmVector<Scalar>);
         m_outputpointer.push_back(NULL);
      }
      m_numinputdata = m_inputpointer.size();
   }

   void addmappeddata(const vistle::shm<Scalar>::array &mapdata){

      m_inputpointer.push_back(mapdata.data());
      m_outData.push_back(new vistle::ShmVector<Scalar>);
      m_outputpointer.push_back(NULL);
      m_numinputdata = m_inputpointer.size();

   }
};

struct DeviceData {

   Scalar m_isovalue;
   Index m_numinputdata;
   IsoDataFunctor m_isoFunc;
   thrust::device_vector<Index> m_el;
   thrust::device_vector<Index> m_cl;
   thrust::device_vector<unsigned char> m_tl;
   thrust::device_vector<Index> m_caseNums;
   thrust::device_vector<Index> m_numVertices;
   thrust::device_vector<Index> m_LocationList;
   thrust::device_vector<Index> m_ValidCellVector;
   thrust::device_vector<Scalar> m_x;
   thrust::device_vector<Scalar> m_y;
   thrust::device_vector<Scalar> m_z;
   std::vector<thrust::device_vector<Scalar> *> m_outData;
   std::vector<thrust::device_ptr<Scalar> > m_inputpointer;
   std::vector<thrust::device_ptr<Scalar> > m_outputpointer;
   typedef thrust::device_vector<Index>::iterator IndexIterator;

   DeviceData(Scalar isoValue
              , IsoDataFunctor isoFunc
              , const vistle::shm<Index>::array &el
              , const vistle::shm<unsigned char>::array &tl
              , const vistle::shm<Index>::array &cl
              , const vistle::shm<Scalar>::array &x
              , const vistle::shm<Scalar>::array &y
              , const vistle::shm<Scalar>::array &z)
   : m_isovalue(isoValue)
   , m_isoFunc(isoFunc)
   , m_el(el.begin(), el.end())
   , m_cl(cl.begin(), cl.end())
   , m_tl(tl.begin(), tl.end())
   , m_x(x.begin(), x.end())
   , m_y(y.begin(), y.end())
   , m_z(z.begin(), z.end())
   {
      m_inputpointer.push_back(m_x.data());
      m_inputpointer.push_back(m_y.data());
      m_inputpointer.push_back(m_z.data());

      for(size_t i = 0; i < m_inputpointer.size(); i++){
         m_outData.push_back(new thrust::device_vector<Scalar>);
      }
      m_outputpointer.resize(m_inputpointer.size());
      m_numinputdata = m_inputpointer.size();
   }
};

template<class Data>
struct process_Cell {
   process_Cell(Data &data) : m_data(data) {
      for (int i = 0; i < m_data.m_numinputdata; i++){
         m_data.m_outputpointer[i] = m_data.m_outData[i]->data();
      }
   }

   Data &m_data;

   __host__ __device__
   void operator()(Index ValidCellIndex) {

      const Index CellNr = m_data.m_ValidCellVector[ValidCellIndex];
      const Index Cellbegin = m_data.m_el[CellNr];
      const Index Cellend = m_data.m_el[CellNr+1];
      const Index numVert = m_data.m_numVertices[ValidCellIndex];
      const auto &cl = &m_data.m_cl[Cellbegin];

      switch (m_data.m_tl[CellNr]) {

         case UnstructuredGrid::HEXAHEDRON: {

            Scalar field[8];
            for (int idx = 0; idx < 8; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

               const int edge = hexaTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

               const int v1 = hexaEdgeTable[0][edge];
               const int v2 = hexaEdgeTable[1][edge];

               Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

               Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

               for(Index j = 0; j < m_data.m_numinputdata; j++){

                  m_data.m_outputpointer[j][outvertexindex] =
                     lerp(m_data.m_inputpointer[j][cl[v1]], m_data.m_inputpointer[j][cl[v2]], t);

               }
            }
            break;
         }

         case UnstructuredGrid::TETRAHEDRON: {

            Scalar field[4];
            for (int idx = 0; idx < 4; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

               const int edge = tetraTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

               const int v1 = tetraEdgeTable[0][edge];
               const int v2 = tetraEdgeTable[1][edge];

               Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

               Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

               for(Index j = 0; j < m_data.m_numinputdata; j++){

                  m_data.m_outputpointer[j][outvertexindex] =
                     lerp(m_data.m_inputpointer[j][cl[v1]], m_data.m_inputpointer[j][cl[v2]], t);

               }
            }
            break;
         }

         case UnstructuredGrid::PYRAMID: {

            Scalar field[5];
            for (int idx = 0; idx < 5; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

               const int edge = pyrTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

               const int v1 = pyrEdgeTable[0][edge];
               const int v2 = pyrEdgeTable[1][edge];

               Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

               Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

               for(Index j = 0; j < m_data.m_numinputdata; j++){

                  m_data.m_outputpointer[j][outvertexindex] =
                     lerp(m_data.m_inputpointer[j][cl[v1]], m_data.m_inputpointer[j][cl[v2]], t);

               }
            }
            break;
         }

         case UnstructuredGrid::PRISM: {

            Scalar field[6];
            for (int idx = 0; idx < 6; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

               const int edge = prismTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

               const int v1 = prismEdgeTable[0][edge];
               const int v2 = prismEdgeTable[1][edge];

               Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

               Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

               for(Index j = 0; j < m_data.m_numinputdata; j++){

                  m_data.m_outputpointer[j][outvertexindex] =
                     lerp(m_data.m_inputpointer[j][cl[v1]], m_data.m_inputpointer[j][cl[v2]], t);

               }
            }
            break;
         }

         case UnstructuredGrid::POLYHEDRON: {

            Index sidebegin = InvalidIndex;
            bool vertexSaved = false;
            Scalar savedData [maxNumdata];
            Index j = 0;
            int flag = 0;
            Scalar middleData[maxNumdata];
            for(int i = 0; i < maxNumdata; i++ ){
               middleData[i] = 0;
            };
            Scalar cd1 [maxNumdata];
            Scalar cd2 [maxNumdata];

            Index outIdx = m_data.m_LocationList[ValidCellIndex];
            for (Index i = Cellbegin; i < Cellend; i++) {

               const Index c1 = m_data.m_cl[i];
               const Index c2 = m_data.m_cl[i+1];

               if (c1 == sidebegin) {

                  sidebegin = InvalidIndex;
                  if (vertexSaved) {

                     for(Index i = 0; i < m_data.m_numinputdata; i++){
                        m_data.m_outputpointer[i][outIdx] = savedData[i];
                     };

                     outIdx += 2;
                     vertexSaved=false;
                  }
                  continue;
               } else if(sidebegin == InvalidIndex) { //Wenn die Neue Seite beginnt

                  flag = 0;
                  sidebegin = c1;
                  vertexSaved = false;
               }

               for(int i = 0; i < m_data.m_numinputdata; i++){
                  cd1[i] = m_data.m_inputpointer[i][c1];
                  cd2[i] = m_data.m_inputpointer[i][c2];
               }

               Scalar d1 = m_data.m_isoFunc(c1);
               Scalar d2 = m_data.m_isoFunc(c2);
               Scalar t = tinterp(m_data.m_isovalue, d1, d2);

               if (d1 <= m_data.m_isovalue && d2 > m_data.m_isovalue) {
                  Scalar v [maxNumdata];
                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     v[i] = lerp(cd1[i], cd2[i], t);
                  };

                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     middleData[i] += v[i];
                  }

                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     m_data.m_outputpointer[i][outIdx] = v[i];
                  }

                  ++outIdx;
                  ++j;
                  flag = 1;

               } else if (d1 > m_data.m_isovalue && d2 <= m_data.m_isovalue) {

                  Scalar v [maxNumdata];

                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     v[i] = lerp(cd1[i], cd2[i], t);
                  };
                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     middleData[i] += v[i];
                  };
                  ++j;
                  if (flag == 1) { //fall 2 nach fall 1
                     for(Index i = 0; i < m_data.m_numinputdata; i++){
                        m_data.m_outputpointer[i][outIdx] = v[i];
                     }
                     outIdx += 2;
                  } else { //fall 2 zuerst

                     for(Index i = 0; i < m_data.m_numinputdata; i++){
                        savedData[i] = v[i];
                     }
                     vertexSaved=true;
                  }
               }
            }
            for(Index i = 0; i < m_data.m_numinputdata; i++){
               middleData[i] /= j;
            };
            for (Index i = 2; i < numVert; i += 3) {
               const Index idx = m_data.m_LocationList[ValidCellIndex]+i;
               for(Index i = 0; i < m_data.m_numinputdata; i++){
                  m_data.m_outputpointer[i][idx] = middleData[i];
               }
            };
            break;
         }
      }
   }
};

template<class Data>
struct checkcell {

   typedef float argument_type;
   typedef float result_type;
   Data &m_data;
   checkcell(Data &data) : m_data(data) {}

   __host__ __device__ int operator()(const thrust::tuple<Index,Index> iCell) const {

      int havelower = 0;
      int havehigher = 0;
      Index Cell = iCell.get<0>();
      Index nextCell = iCell.get<1>();
      for (Index i=Cell; i<nextCell; i++) {
         float val = m_data.m_isoFunc(m_data.m_cl[i]);
         if (val>m_data.m_isovalue) {
            havelower=1;
            if (havehigher)
               return 1;
         } else {
            havehigher=1;
            if (havelower)
               return 1;
         }
      }
      return 0;
   }
};

template<class Data>
struct classify_cell {

   classify_cell(Data &data) : m_data(data) {}

   Data &m_data;

   __host__ __device__ thrust::tuple<Index,Index> operator()(Index CellNr) {

      uint tableIndex = 0;
      Index Start = m_data.m_el[CellNr];
      Index diff = m_data.m_el[CellNr+1]-Start;
      unsigned char CellType = m_data.m_tl[CellNr];
      int numVerts = 0;
      if (CellType != UnstructuredGrid::POLYHEDRON) {
         for (int idx = 0; idx < diff; idx ++) {
            tableIndex += (((int) (m_data.m_isoFunc(m_data.m_cl[Start+idx]) > m_data.m_isovalue)) << idx);
         }
      }
      switch (CellType) {

         case UnstructuredGrid::HEXAHEDRON:
            numVerts = hexaNumVertsTable[tableIndex];
            break;

         case UnstructuredGrid::TETRAHEDRON:
            numVerts = tetraNumVertsTable[tableIndex];
            break;

         case UnstructuredGrid::PYRAMID:
            numVerts = pyrNumVertsTable[tableIndex];
            break;

         case UnstructuredGrid::PRISM:
            numVerts = prismNumVertsTable[tableIndex];
            break;

         case UnstructuredGrid::POLYHEDRON: {

            Index sidebegin = InvalidIndex;
            Index vertcounter = 0;
            for (Index i = Start; i < Start + diff; i++) {

               if (m_data.m_cl[i] == sidebegin) {
                  sidebegin = InvalidIndex;
                  continue;
               }

               if (sidebegin == InvalidIndex) {
                  sidebegin = m_data.m_cl[i];
               }

               if (m_data.m_isoFunc(m_data.m_cl[i]) <= m_data.m_isovalue && m_data.m_isoFunc(m_data.m_cl[i+1]) > m_data.m_isovalue) {

                  vertcounter += 1;
               } else if(m_data.m_isoFunc(m_data.m_cl[i]) > m_data.m_isovalue && m_data.m_isoFunc(m_data.m_cl[i+1]) <= m_data.m_isovalue) {

                  vertcounter += 1;
               }
            }
            numVerts = vertcounter + vertcounter/2;
            break;
         }
      };
      return thrust::make_tuple<Index, Index> (tableIndex, numVerts);
   }
};

Leveller::Leveller(UnstructuredGrid::const_ptr grid, const Scalar isovalue, Index processortype
         #ifdef CUTTINGSURFACE
            , int option
         #endif
            )
      : m_grid(grid)
   #ifdef CUTTINGSURFACE
      , m_option(option)
   #endif
      , m_isoValue(isovalue)
      , m_processortype(processortype)
      , gmin(std::numeric_limits<Scalar>::max())
      , gmax(-std::numeric_limits<Scalar>::max())
   {
      m_triangles = Triangles::ptr(new Triangles(Object::Initialized));
      m_triangles->setMeta(grid->meta());
   }

template<class Data, class pol>
Index Leveller::calculateSurface(Data &data) {

   thrust::counting_iterator<int> first(0);
   thrust::counting_iterator<int> last = first + m_grid->getNumElements();
   typedef thrust::tuple<typename Data::IndexIterator, typename Data::IndexIterator> Iteratortuple;
   typedef thrust::zip_iterator<Iteratortuple> ZipIterator;
   ZipIterator ElTupleVec(thrust::make_tuple(data.m_el.begin(), data.m_el.begin()+1));
   data.m_ValidCellVector.resize(m_grid->getNumElements());
   typename Data::VectorIndexIterator end = thrust::copy_if(pol(), first, last, ElTupleVec, data.m_ValidCellVector.begin(), checkcell<Data>(data));
   size_t numValidCells = end-data.m_ValidCellVector.begin();
   data.m_caseNums.resize(numValidCells);
   data.m_numVertices.resize(numValidCells);
   data.m_LocationList.resize(numValidCells);
   thrust::transform(pol(), data.m_ValidCellVector.begin(), end, thrust::make_zip_iterator(thrust::make_tuple(data.m_caseNums.begin(),data.m_numVertices.begin())), classify_cell<Data>(data));
   thrust::exclusive_scan(pol(), data.m_numVertices.begin(), data.m_numVertices.end(), data.m_LocationList.begin());
   Index totalNumVertices = 0;
   if (!data.m_numVertices.empty())
      totalNumVertices += data.m_numVertices.back();
   if (!data.m_LocationList.empty())
      totalNumVertices += data.m_LocationList.back();
   for(int i = 0; i < data.m_numinputdata; i++){
      data.m_outData[i]->resize(totalNumVertices);
   };
   thrust::counting_iterator<Index> start(0), finish(numValidCells);
   thrust::for_each(pol(), start, finish, process_Cell<Data>(data));
   return totalNumVertices;
}

bool Leveller::process() {
   if(m_mapdata.size()){
      Vec<Scalar>::const_ptr mapdataobj = Vec<Scalar>::as(m_mapdata[0]);
   }
#ifndef CUTTINGSURFACE
   Vec<Scalar>::const_ptr dataobj = Vec<Scalar>::as(m_data);
   if (!dataobj)
      return false;
#else
#endif

   Index totalNumVertices = 0;

   switch (m_processortype) {

      case Host: {

         HostData HD(m_isoValue,
#ifndef CUTTINGSURFACE
               IsoDataFunctor(dataobj->x().data()),
#else
               IsoDataFunctor(vertex, point, direction, m_grid->x().data(), m_grid->y().data(), m_grid->z().data(), m_option),
#endif
               m_grid->el(), m_grid->tl(), m_grid->cl(), m_grid->x(), m_grid->y(), m_grid->z());

         if(m_mapdata.size()){
            if(Vec<Scalar,1>::const_ptr Scal = Vec<Scalar,1>::as(m_mapdata[0])){
               HD.addmappeddata(Scal->x());
            }
            if(Vec<Scalar,3>::const_ptr Vect = Vec<Scalar,3>::as(m_mapdata[0])){
               HD.addmappeddata(Vect->x());
               HD.addmappeddata(Vect->y());
               HD.addmappeddata(Vect->z());
            }

         }

         totalNumVertices = calculateSurface<HostData, thrust::detail::host_t>(HD);

         m_triangles->d()->x[0] = HD.m_outData[0];
         m_triangles->d()->x[1] = HD.m_outData[1];
         m_triangles->d()->x[2] = HD.m_outData[2];

         if(m_mapdata.size()){
            if(Vec<Scalar>::as(m_mapdata[0])){

               Vec<Scalar,1>::ptr out = Vec<Scalar,1>::ptr(new Vec<Scalar,1>(Object::Initialized));
               out->d()->x[0] = HD.m_outData[3];
               m_outmapData.push_back(out);

            }
            if(Vec<Scalar,3>::as(m_mapdata[0])){

               Vec<Scalar,3>::ptr out = Vec<Scalar,3>::ptr(new Vec<Scalar,3>(Object::Initialized));
               out->d()->x[0] = HD.m_outData[3];
               out->d()->x[1] = HD.m_outData[4];
               out->d()->x[2] = HD.m_outData[5];
               m_outmapData.push_back(out);

            }

            m_outmapData.back()->setMeta(m_mapdata[0]->meta());
         }
         break;
      }

      case Device: {

         DeviceData DD(m_isoValue,
#ifndef CUTTINGSURFACE
               IsoDataFunctor(dataobj->x().data()),
#else
               IsoDataFunctor(vertex, point, direction, m_grid->x().data(), m_grid->y().data(), m_grid->z().data(), m_option),
#endif
               m_grid->el(), m_grid->tl(), m_grid->cl(), m_grid->x(), m_grid->y(), m_grid->z());

#if 0
         totalNumVertices = calculateSurface<DeviceData, thrust::device>(DD);
#endif

         m_triangles->x().resize(totalNumVertices);
         Scalar *out_x = m_triangles->x().data();
         thrust::copy(DD.m_outData[0]->begin(), DD.m_outData[0]->end(), out_x);

         m_triangles->y().resize(totalNumVertices);
         Scalar *out_y = m_triangles->y().data();
         thrust::copy(DD.m_outData[1]->begin(), DD.m_outData[1]->end(), out_y);

         m_triangles->z().resize(totalNumVertices);
         Scalar *out_z = m_triangles->z().data();
         thrust::copy(DD.m_outData[2]->begin(), DD.m_outData[2]->end(), out_z);

         if(m_mapdata.size()){
            if(Vec<Scalar>::as(m_mapdata[0])){

               Vec<Scalar>::ptr out = Vec<Scalar>::ptr(new Vec<Scalar>(totalNumVertices));
               thrust::copy(DD.m_outData[3]->begin(), DD.m_outData[3]->end(), out->x().data());
               m_outmapData.push_back(out);

            }
            if(Vec<Scalar,3>::as(m_mapdata[0])){

               Vec<Scalar,3>::ptr out = Vec<Scalar,3>::ptr(new Vec<Scalar,3>(totalNumVertices));
               thrust::copy(DD.m_outData[3]->begin(), DD.m_outData[3]->end(), out->x().data());
               thrust::copy(DD.m_outData[4]->begin(), DD.m_outData[4]->end(), out->y().data());
               thrust::copy(DD.m_outData[5]->begin(), DD.m_outData[5]->end(), out->z().data());
               m_outmapData.push_back(out);

            }

            m_outmapData.back()->setMeta(m_mapdata[0]->meta());
         }
         break;
      }
   }
   m_triangles->cl().resize(totalNumVertices);
   Index *out_cl = m_triangles->cl().data();
   thrust::counting_iterator<Index> first(0), last(totalNumVertices);
   thrust::copy(first, last, out_cl);

   return true;
}

#ifdef CUTTINGSURFACE
void Leveller::setCutData(const Vector m_vertex, const Vector m_point, const Vector m_direction){
   vertex = m_vertex;
   point = m_point;
   direction = m_direction;
}
#else    
void Leveller::setIsoData(Vec<Scalar>::const_ptr obj) {
   m_data = obj;
}
#endif

void Leveller::addMappedData(Object::const_ptr mapobj ){
   m_mapdata.push_back(mapobj);
}

Object::ptr Leveller::result() {
      return m_triangles;
   }

Object::ptr Leveller::mapresult() {
   if(m_outmapData.size())
      return m_outmapData[0];
   else
      return Object::ptr(new Empty(Object::Initialized));
}

std::pair<Scalar, Scalar> Leveller::range() {
   return std::make_pair(gmin, gmax);
}
