//
//This code is used for both IsoCut and IsoSurface!
//

#include <sstream>
#include <iomanip>
#include <core/index.h>
#include <core/scalar.h>
#include <core/unstr.h>
#include <core/triangles.h>
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

template<typename S>
inline S lerp(S a, S b, Scalar t) {
    return a+t*(b-a);
}

template<Index>
inline Index lerp(Index a, Index b, Scalar t) {
    return t > 0.5 ? b : a;
}


const Scalar EPSILON = 1.0e-10f;
const int MaxNumData = 6;

inline Scalar __host__ __device__ tinterp(Scalar iso, const Scalar &f0, const Scalar &f1) {

   const Scalar diff = (f1 - f0);
   const Scalar d0 = iso - f0;
   if (fabs(diff) < EPSILON) {
       const Scalar d1 = f1 - iso;
      return fabs(d0) < fabs(d1) ? 1 : 0;
   }

   return std::min(Scalar(1), std::max(Scalar(0), d0 / diff));
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
   Index m_numinputdata, m_numinputdataI;
   IsoDataFunctor m_isoFunc;
   const Index *m_el;
   const Index *m_cl;
   const unsigned char *m_tl;
   std::vector<Index> m_caseNums;
   std::vector<Index> m_numVertices;
   std::vector<Index> m_LocationList;
   std::vector<Index> m_ValidCellVector;
   const Scalar *m_x;
   const Scalar *m_y;
   const Scalar *m_z;
   std::vector<vistle::shm_ref<vistle::shm_array<Scalar, shm<Scalar>::allocator>>> m_outData;
   std::vector<vistle::shm_ref<vistle::shm_array<Index, shm<Index>::allocator>>> m_outDataI;
   std::vector<const Scalar*> m_inputpointer;
   std::vector<const Index*> m_inputpointerI;
   std::vector<Scalar*> m_outputpointer;
   std::vector<Index *> m_outputpointerI;

   typedef const Index *IndexIterator;
   typedef std::vector<Index>::iterator VectorIndexIterator;

   HostData(Scalar isoValue
            , IsoDataFunctor isoFunc
            , const Index *el
            , const unsigned char *tl
            , const Index *cl
            , const Scalar *x
            , const Scalar *y
            , const Scalar *z
            )
      : m_isovalue(isoValue)
      , m_numinputdata(0)
      , m_numinputdataI(0)
      , m_isoFunc(isoFunc)
      , m_el(el)
      , m_cl(cl)
      , m_tl(tl)
      , m_x(x)
      , m_y(y)
      , m_z(z)
   {
      m_inputpointer.push_back(&x[0]);
      m_inputpointer.push_back(&y[0]);
      m_inputpointer.push_back(&z[0]);

      for(size_t i = 0; i < m_inputpointer.size(); i++){
         m_outData.emplace_back(vistle::ShmVector<Scalar>::create(0));
         m_outputpointer.push_back(NULL);
      }
      m_numinputdata = m_inputpointer.size();
   }

   void addmappeddata(const Scalar *mapdata){

      m_inputpointer.push_back(mapdata);
      m_outData.push_back(vistle::ShmVector<Scalar>::create(0));
      m_outputpointer.push_back(NULL);
      m_numinputdata = m_inputpointer.size();

   }

   void addmappeddata(const Index *mapdata){

      m_inputpointerI.push_back(mapdata);
      m_outDataI.push_back(vistle::ShmVector<Index>::create(0));
      m_outputpointerI.push_back(NULL);
      m_numinputdataI = m_inputpointerI.size();

   }
};

struct DeviceData {

   Scalar m_isovalue;
   Index m_numinputdata, m_numinputdataI;
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
   std::vector<thrust::device_vector<Index> *> m_outDataI;
   std::vector<thrust::device_ptr<Scalar> > m_inputpointer;
   std::vector<thrust::device_ptr<Index> > m_inputpointerI;
   std::vector<thrust::device_ptr<Scalar> > m_outputpointer;
   std::vector<thrust::device_ptr<Index> > m_outputpointerI;
   typedef const Index *IndexIterator;
   //typedef thrust::device_vector<Index>::iterator IndexIterator;

   DeviceData(Scalar isoValue
              , IsoDataFunctor isoFunc
              , Index nelem
              , const Index *el
              , const unsigned char *tl
              , Index nconn
              , const Index *cl
              , Index ncoord
              , const Scalar *x
              , const Scalar *y
              , const Scalar *z)
   : m_isovalue(isoValue)
   , m_isoFunc(isoFunc)
   , m_el(el, el+nelem)
   , m_cl(cl, cl+nconn)
   , m_tl(tl, tl+nelem)
   , m_x(x, x+ncoord)
   , m_y(y, y+ncoord)
   , m_z(z, z+ncoord)
   {
      m_inputpointer.push_back(m_x.data());
      m_inputpointer.push_back(m_y.data());
      m_inputpointer.push_back(m_z.data());

      for(size_t i = 0; i < m_inputpointer.size(); i++){
         m_outData.push_back(new thrust::device_vector<Scalar>);
      }
      m_outputpointer.resize(m_inputpointer.size());
      m_numinputdata = m_inputpointer.size();
      for(size_t i = 0; i < m_inputpointerI.size(); i++){
         m_outDataI.push_back(new thrust::device_vector<Index>);
      }
      m_outputpointerI.resize(m_inputpointerI.size());
      m_numinputdataI = m_inputpointerI.size();
   }
};

template<class Data>
struct process_Cell {
   process_Cell(Data &data) : m_data(data) {
      for (int i = 0; i < m_data.m_numinputdata; i++){
         m_data.m_outputpointer[i] = m_data.m_outData[i]->data();
      }
      for (int i = 0; i < m_data.m_numinputdataI; i++){
         m_data.m_outputpointerI[i] = m_data.m_outDataI[i]->data();
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

#define INTER(triTable, edgeTable) \
    const unsigned int edge = triTable[m_data.m_caseNums[ValidCellIndex]][idx]; \
    const unsigned int v1 = edgeTable[0][edge]; \
    const unsigned int v2 = edgeTable[1][edge]; \
    const Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]); \
    Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx; \
    for(Index j = 0; j < m_data.m_numinputdata; j++) { \
        m_data.m_outputpointer[j][outvertexindex] = \
            lerp(m_data.m_inputpointer[j][cl[v1]], m_data.m_inputpointer[j][cl[v2]], t); \
    } \
    for(Index j = 0; j < m_data.m_numinputdataI; j++) { \
        m_data.m_outputpointerI[j][outvertexindex] = \
            lerp(m_data.m_inputpointerI[j][cl[v1]], m_data.m_inputpointerI[j][cl[v2]], t); \
    }

      switch (m_data.m_tl[CellNr]) {

         case UnstructuredGrid::HEXAHEDRON: {

            Scalar field[8];
            for (int idx = 0; idx < 8; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
               INTER(hexaTriTable, hexaEdgeTable);
            }
            break;
         }

         case UnstructuredGrid::TETRAHEDRON: {

            Scalar field[4];
            for (int idx = 0; idx < 4; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
               INTER(tetraTriTable, tetraEdgeTable);
            }
            break;
         }

         case UnstructuredGrid::PYRAMID: {

            Scalar field[5];
            for (int idx = 0; idx < 5; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                INTER(pyrTriTable, pyrEdgeTable);
            }
            break;
         }

         case UnstructuredGrid::PRISM: {

            Scalar field[6];
            for (int idx = 0; idx < 6; idx ++) {
               field[idx] = m_data.m_isoFunc(cl[idx]);
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                INTER(prismTriTable, prismEdgeTable);
            }
            break;
         }

         case UnstructuredGrid::POLYHEDRON: {

            Index sidebegin = InvalidIndex;
            bool vertexSaved = false;
            Scalar savedData [MaxNumData];
            Index savedDataI[MaxNumData];
            Index numTri = 0;
            int flag = 0;
            Scalar middleData[MaxNumData];
            Index middleDataI[MaxNumData];
            for(int i = 0; i < MaxNumData; i++ ){
               middleData[i] = 0;
               middleDataI[i] = 0;
            };
            Scalar cd1[MaxNumData], cd2[MaxNumData];
            Index cd1I[MaxNumData], cd2I[MaxNumData];

            Index outIdx = m_data.m_LocationList[ValidCellIndex];
            for (Index i = Cellbegin; i < Cellend; i++) {

               const Index c1 = m_data.m_cl[i];
               const Index c2 = m_data.m_cl[i+1];

               if (c1 == sidebegin) {

                  sidebegin = InvalidIndex;
                  if (vertexSaved) {

                     for(Index i = 0; i < m_data.m_numinputdata; i++){
                        m_data.m_outputpointer[i][outIdx] = savedData[i];
                     }
                     for(Index i = 0; i < m_data.m_numinputdataI; i++){
                        m_data.m_outputpointerI[i][outIdx] = savedDataI[i];
                     }

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
               for(int i = 0; i < m_data.m_numinputdataI; i++){
                  cd1I[i] = m_data.m_inputpointerI[i][c1];
                  cd2I[i] = m_data.m_inputpointerI[i][c2];
               }

               Scalar d1 = m_data.m_isoFunc(c1);
               Scalar d2 = m_data.m_isoFunc(c2);
               Scalar t = tinterp(m_data.m_isovalue, d1, d2);

               if (d1 <= m_data.m_isovalue && d2 > m_data.m_isovalue) {
                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     Scalar v = lerp(cd1[i], cd2[i], t);
                     middleData[i] += v;
                     m_data.m_outputpointer[i][outIdx] = v;
                  }
                  for(Index i = 0; i < m_data.m_numinputdataI; i++){
                     Index vI = lerp(cd1I[i], cd2I[i], t);
                     middleDataI[i] += vI;
                     m_data.m_outputpointerI[i][outIdx] = vI;
                  };

                  ++outIdx;
                  ++numTri;
                  flag = 1;

               } else if (d1 > m_data.m_isovalue && d2 <= m_data.m_isovalue) {

                  Scalar v [MaxNumData];
                  Index vI[MaxNumData];

                  for(Index i = 0; i < m_data.m_numinputdata; i++){
                     v[i] = lerp(cd1[i], cd2[i], t);
                     middleData[i] += v[i];
                  }
                  for(Index i = 0; i < m_data.m_numinputdataI; i++){
                     vI[i] = lerp(cd1I[i], cd2I[i], t);
                     middleDataI[i] += vI[i];
                  }
                  ++numTri;
                  if (flag == 1) { //fall 2 nach fall 1
                     for(Index i = 0; i < m_data.m_numinputdata; i++){
                        m_data.m_outputpointer[i][outIdx] = v[i];
                     }
                     for(Index i = 0; i < m_data.m_numinputdataI; i++){
                        m_data.m_outputpointerI[i][outIdx] = vI[i];
                     }
                     outIdx += 2;
                  } else { //fall 2 zuerst

                     for(Index i = 0; i < m_data.m_numinputdata; i++){
                        savedData[i] = v[i];
                     }
                     for(Index i = 0; i < m_data.m_numinputdataI; i++){
                        savedDataI[i] = vI[i];
                     }
                     vertexSaved=true;
                  }
               }
            }
            if (numTri > 0) {
                for(Index i = 0; i < m_data.m_numinputdata; i++){
                    middleData[i] /= numTri;
                }
                for(Index i = 0; i < m_data.m_numinputdataI; i++){
                    middleDataI[i] /= numTri;
                }
            }
            for (Index i = 2; i < numVert; i += 3) {
               const Index idx = m_data.m_LocationList[ValidCellIndex]+i;
               for(Index i = 0; i < m_data.m_numinputdata; i++){
                  m_data.m_outputpointer[i][idx] = middleData[i];
               }
               for(Index i = 0; i < m_data.m_numinputdataI; i++){
                  m_data.m_outputpointerI[i][idx] = middleDataI[i];
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
   ZipIterator ElTupleVec(thrust::make_tuple(&data.m_el[0], &data.m_el[1]));
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
   }
   for(int i = 0; i < data.m_numinputdataI; i++){
      data.m_outDataI[i]->resize(totalNumVertices);
   }
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
               IsoDataFunctor(&dataobj->x()[0]),
#else
               IsoDataFunctor(vertex, point, direction, &m_grid->x()[0], &m_grid->y()[0], &m_grid->z()[0], m_option),
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
            if(Vec<Index,1>::const_ptr Idx = Vec<Index,1>::as(m_mapdata[0])){
               HD.addmappeddata(Idx->x());
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
               out->setMeta(m_mapdata[0]->meta());
               m_outmapData.push_back(out);

            }
            if(Vec<Scalar,3>::as(m_mapdata[0])){

               Vec<Scalar,3>::ptr out = Vec<Scalar,3>::ptr(new Vec<Scalar,3>(Object::Initialized));
               out->d()->x[0] = HD.m_outData[3];
               out->d()->x[1] = HD.m_outData[4];
               out->d()->x[2] = HD.m_outData[5];
               out->setMeta(m_mapdata[0]->meta());
               m_outmapData.push_back(out);

            }
            if(Vec<Index>::as(m_mapdata[0])){

               Vec<Index>::ptr out = Vec<Index>::ptr(new Vec<Index>(Object::Initialized));
               out->d()->x[0] = HD.m_outDataI[0];
               out->setMeta(m_mapdata[0]->meta());
               m_outmapData.push_back(out);

            }
         }
         break;
      }

      case Device: {

         DeviceData DD(m_isoValue,
#ifndef CUTTINGSURFACE
               IsoDataFunctor(&dataobj->x()[0]),
#else
               IsoDataFunctor(vertex, point, direction, &m_grid->x()[0], &m_grid->y()[0], &m_grid->z()[0], m_option),
#endif
               m_grid->getNumElements(), m_grid->el(), m_grid->tl(), m_grid->getNumCorners(), m_grid->cl(), m_grid->getSize(), m_grid->x(), m_grid->y(), m_grid->z());

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
               out->setMeta(m_mapdata[0]->meta());
               m_outmapData.push_back(out);

            }
            if(Vec<Scalar,3>::as(m_mapdata[0])){

               Vec<Scalar,3>::ptr out = Vec<Scalar,3>::ptr(new Vec<Scalar,3>(totalNumVertices));
               thrust::copy(DD.m_outData[3]->begin(), DD.m_outData[3]->end(), out->x().data());
               thrust::copy(DD.m_outData[4]->begin(), DD.m_outData[4]->end(), out->y().data());
               thrust::copy(DD.m_outData[5]->begin(), DD.m_outData[5]->end(), out->z().data());
               out->setMeta(m_mapdata[0]->meta());
               m_outmapData.push_back(out);

            }
         }
         break;
      }
   }

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

DataBase::ptr Leveller::mapresult() {
   if(m_outmapData.size())
      return m_outmapData[0];
   else
      return DataBase::ptr();
}

std::pair<Scalar, Scalar> Leveller::range() {
   return std::make_pair(gmin, gmax);
}
