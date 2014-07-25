#include <sstream>
#include <iomanip>
#include <core/message.h>
#include <core/object.h>
#include <core/unstr.h>
#include <core/vec.h>
#include <core/triangles.h>
#include <core/vector.h>
#include <boost/mpi/collectives.hpp>
#include "tables.h"
#include "IsoSurface.h"
#include <thrust/device_vector.h>
#include <thrust/transform.h>
#include <thrust/fill.h>
#include <thrust/replace.h>
#include <thrust/functional.h>
#include <thrust/scan.h>
#include <iostream>
#include <cmath>
#include <thrust/sequence.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <ctime>
#include <stdio.h>
#include <time.h>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/tuple.h>

//#ifdef HAVE_CUDA
//#include <thrust/system/cuda/execution_policy.h>
//#include <thrust/system/cuda/vector.h>
//#endif

#ifdef USE_OMP
#ifndef _OPENMP
#error "no OpenMP support"
#endif
#include <thrust/system/omp/vector.h>
#include <thrust/system/omp/execution_policy.h>
#endif

#ifdef USE_TBB
#include <thrust/system/tbb/vector.h>
#include <thrust/system/tbb/execution_policy.h>
#endif

#ifdef USE_CPP
#include <thrust/system/cpp/vector.h>
#include <thrust/system/cpp/execution_policy.h>
#endif

MODULE_MAIN(IsoSurface)

using namespace vistle;


#define lerp(a, b, t) ( a + t * (b - a) )

const Scalar EPSILON = 1.0e-10f;

inline Vector lerp3(const Vector &a, const Vector &b, const Scalar t) {

   return lerp(a, b, t);

}

inline Scalar cvlerp3(const Scalar &a, const Scalar &b, const Scalar t) {

    return lerp(a, b, t);

}


inline Scalar tinterp(Scalar iso, const Scalar &f0, const Scalar &f1) {

   Scalar diff = (f1 - f0);

   if (fabs(diff) < EPSILON)
      return 0;

   if (fabs(iso - f0) < EPSILON)
      return 0;

   if (fabs(iso - f1) < EPSILON)
      return 1;

   Scalar t = (iso - f0) / diff;

   return t;
}

inline Vector interp(Scalar iso, const Vector &p0, const Vector &p1, const Scalar &f0, const Scalar &f1) {

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

inline Scalar cvinterp(Scalar iso, const Scalar &f0, const Scalar &f1, const Scalar &f2, const Scalar &f3) {

   Scalar diff = (f1 - f0);

   if (fabs(diff) < EPSILON)
      return f2;

   if (fabs(iso - f0) < EPSILON)
      return f2;

   if (fabs(iso - f1) < EPSILON)
      return f3;

   Scalar t = (iso - f0) / diff;

   return cvlerp3(f2, f3, t);
}

struct HostData {

   Scalar m_isovalue;
   Index m_numinputdata;
   const vistle::shm<Scalar>::array &m_volumedata;
   const vistle::shm<Scalar>::array &m_volumemapdata;
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
   vistle::ShmVector<Scalar>::ptr m_xCoordinateVector;
   vistle::ShmVector<Scalar>::ptr m_yCoordinateVector;
   vistle::ShmVector<Scalar>::ptr m_zCoordinateVector;
   vistle::ShmVector<Scalar>::ptr m_mapdataVector;   
   std::vector<vistle::ShmVector<Scalar>::ptr> m_outData;
   std::vector<const vistle::shm<Scalar>::array *> m_InData;

   std::vector<Scalar*> m_inputpointer;
   std::vector<Scalar*> m_outputpointer;

   Scalar* m_xpointer;
   Scalar* m_ypointer;
   Scalar* m_zpointer;
   Scalar* m_mapdatapointer;
   Scalar* m_datapointer;

   typedef vistle::shm<Index>::array::iterator Indexiterator;

   HostData(Scalar isoValue

         , const vistle::shm<Scalar>::array &data
         , const vistle::shm<Scalar>::array &mapdata
         , const vistle::shm<Index>::array &el
         , const vistle::shm<unsigned char>::array &tl
         , const vistle::shm<Index>::array &cl
         , const vistle::shm<Scalar>::array &x
         , const vistle::shm<Scalar>::array &y
         , const vistle::shm<Scalar>::array &z
         ,Index inputdim)

      : m_isovalue(isoValue)
      , m_volumedata(data)
      , m_volumemapdata(mapdata)
      , m_el(el)
      , m_cl(cl)
      , m_tl(tl)
      , m_x(x)
      , m_y(y)
      , m_z(z)
      , m_numinputdata(inputdim) {

         m_xCoordinateVector = new vistle::ShmVector<Scalar>;
         m_yCoordinateVector = new vistle::ShmVector<Scalar>;
         m_zCoordinateVector = new vistle::ShmVector<Scalar>;
         m_mapdataVector = new vistle::ShmVector<Scalar>;
         m_InData.push_back(&x);
         m_InData.push_back(&y);
         m_InData.push_back(&z);
         m_InData.push_back(&mapdata);


         m_inputpointer.push_back(x.data());
         m_inputpointer.push_back(y.data());
         m_inputpointer.push_back(z.data());
         m_inputpointer.push_back(mapdata.data());


         for(int i = 0; i < inputdim; i++){
             m_outData.push_back(new vistle::ShmVector<Scalar>);
             m_outputpointer.push_back(new Scalar);
         }

   }

   std::vector<Scalar> xCoordinateVector;
   std::vector<Scalar> yCoordinateVector;
   std::vector<Scalar> zCoordinateVector;
   std::vector<Scalar> mapdataVector;
   std::vector<std::vector<Scalar>> outData;
};

struct DeviceData {

   Scalar m_isovalue;
   thrust::device_vector<Scalar> m_volumedata;
   thrust::device_vector<Scalar> m_volumemapdata;
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
   thrust::device_vector<Scalar> m_xCoordinateVector;
   thrust::device_vector<Scalar> m_yCoordinateVector;
   thrust::device_vector<Scalar> m_zCoordinateVector;

   Scalar* m_xpointer;
   Scalar* m_ypointer;
   Scalar* m_zpointer;
   Scalar* m_mapdatapointer;
   typedef thrust::device_vector<Index>::iterator Indexiterator;

   DeviceData(Scalar isoValue
         , const vistle::shm<Scalar>::array &data
         , const vistle::shm<Scalar>::array &mapdata
         , const vistle::shm<Index>::array &el
         , const vistle::shm<unsigned char>::array &tl
         , const vistle::shm<Index>::array &cl
         , const vistle::shm<Scalar>::array &x
         , const vistle::shm<Scalar>::array &y
         , const vistle::shm<Scalar>::array &z)
      : m_isovalue(isoValue) {

         m_volumedata.resize(data.size());
         for (Index i = 0; i < data.size(); i++) {
            m_volumedata[i] = data[i];
         }
         m_volumemapdata.resize(mapdata.size());
         for (Index i = 0; i < mapdata.size(); i++) {
            m_volumemapdata[i] = mapdata[i];
         }
         m_el.resize(el.size());
         for (Index i = 0; i < el.size(); i++) {
            m_el[i]=el[i];
         }
         m_cl.resize(cl.size());
         for (Index i = 0; i < cl.size(); i++) {
            m_cl[i]=cl[i];
         }
         m_tl.resize(tl.size());
         for (Index i = 0; i < tl.size(); i++) {
            m_tl[i] = tl[i];
         }
         m_x.resize(x.size());
         for (Index i = 0; i < x.size(); i++) {
            m_x[i] = x[i];
         }
         m_y.resize(y.size());
         for (Index i = 0; i < y.size(); i++) {
            m_y[i] = y[i];
         }
         m_z.resize(z.size());
         for (Index i = 0; i < z.size(); i++) {
            m_z[i] = z[i];
         }
      }
};

template<class Data>
struct process_Cell {
   process_Cell(Data &data) : m_data(data) {
       for (int i = 0; i < 4; i++){
           m_data.m_outputpointer[i] = m_data.m_outData[i]->data();
        }
   }

   Data &m_data;

   __host__ __device__
   void operator()(Index ValidCellIndex) {

      Index CellNr = m_data.m_ValidCellVector[ValidCellIndex];
      Index Cellbegin = m_data.m_el[CellNr];
      Index Cellend = m_data.m_el[CellNr+1];
      Index numVert = m_data.m_numVertices[ValidCellIndex];

      switch (m_data.m_tl[CellNr]) {

      case UnstructuredGrid::HEXAHEDRON: {

          Index newindex[8];
          newindex[0] = m_data.m_cl[Cellbegin];
          newindex[1] = m_data.m_cl[Cellbegin +1];
          newindex[2] = m_data.m_cl[Cellbegin +2];
          newindex[3] = m_data.m_cl[Cellbegin +3];
          newindex[4] = m_data.m_cl[Cellbegin +4];
          newindex[5] = m_data.m_cl[Cellbegin +5];
          newindex[6] = m_data.m_cl[Cellbegin +6];
          newindex[7] = m_data.m_cl[Cellbegin +7];

          Scalar field[8];
          for (int idx = 0; idx < 8; idx ++) {
              field[idx] = m_data.m_volumedata[newindex[idx]];
          }

          for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

              const int edge = hexaTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

              const int v1 = hexaEdgeTable[0][edge];
              const int v2 = hexaEdgeTable[1][edge];

              Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

              Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

              for(Index j = 0; j < 4; j++){

                  m_data.m_outputpointer[j][outvertexindex] = lerp(m_data.m_inputpointer[j][newindex[v1]], m_data.m_inputpointer[j][newindex[v2]], t);

              }

          }
      }
         break;

      case UnstructuredGrid::TETRAHEDRON: {
          
          Index newindex[8];
          newindex[0] = m_data.m_cl[Cellbegin];
          newindex[1] = m_data.m_cl[Cellbegin +1];
          newindex[2] = m_data.m_cl[Cellbegin +2];
          newindex[3] = m_data.m_cl[Cellbegin +3];


          Scalar field[4];
          for (int idx = 0; idx < 4; idx ++) {
              field[idx] = m_data.m_volumedata[newindex[idx]];
          }

          for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

              const int edge = tetraTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

              const int v1 = tetraEdgeTable[0][edge];
              const int v2 = tetraEdgeTable[1][edge];

              Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

              Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

              for(Index j = 0; j < 4; j++){

                  m_data.m_outputpointer[j][outvertexindex] = lerp(m_data.m_inputpointer[j][newindex[v1]], m_data.m_inputpointer[j][newindex[v2]], t);

              }
          }
      }
         break;

         case UnstructuredGrid::PYRAMID: {

            Index newindex[5];
            newindex[0] = m_data.m_cl[Cellbegin ];
            newindex[1] = m_data.m_cl[Cellbegin +1];
            newindex[2] = m_data.m_cl[Cellbegin +2];
            newindex[3] = m_data.m_cl[Cellbegin +3];
            newindex[4] = m_data.m_cl[Cellbegin +4];

            Scalar field[5];
            for (int idx = 0; idx < 5; idx ++) {
               field[idx] = m_data.m_volumedata[newindex[idx]];
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

                const int edge = pyrTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

                const int v1 = pyrEdgeTable[0][edge];
                const int v2 = pyrEdgeTable[1][edge];

                Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

                Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

                for(Index j = 0; j < 4; j++){

                    m_data.m_outputpointer[j][outvertexindex] = lerp(m_data.m_inputpointer[j][newindex[v1]], m_data.m_inputpointer[j][newindex[v2]], t);

                }
            }
         }
         break;

         case UnstructuredGrid::PRISM: {

            Index newindex[6];
            newindex[0] = m_data.m_cl[Cellbegin ];
            newindex[1] = m_data.m_cl[Cellbegin +1];
            newindex[2] = m_data.m_cl[Cellbegin +2];
            newindex[3] = m_data.m_cl[Cellbegin +3];
            newindex[4] = m_data.m_cl[Cellbegin +4];
            newindex[5] = m_data.m_cl[Cellbegin +5];

            Scalar field[6];
            for (int idx = 0; idx < 6; idx ++) {
               field[idx] = m_data.m_volumedata[newindex[idx]];
            }

            for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {

                const int edge = prismTriTable[m_data.m_caseNums[ValidCellIndex]][idx];

                const int v1 = prismEdgeTable[0][edge];
                const int v2 = prismEdgeTable[1][edge];

                Scalar t = tinterp(m_data.m_isovalue, field[v1], field[v2]);

                Index outvertexindex = m_data.m_LocationList[ValidCellIndex]+idx;

                for(Index j = 0; j < 4; j++){

                    m_data.m_outputpointer[j][outvertexindex] = lerp(m_data.m_inputpointer[j][newindex[v1]], m_data.m_inputpointer[j][newindex[v2]], t);

                }
            }
         }
         break;

         case UnstructuredGrid::POLYHEDRON: {

            SIndex sidebegin = -1;
            Vector middleVector(0,0,0);

            bool vertexSaved=false;
            Vector savedVertex;
            Index numdataonly = m_data.m_numinputdata - 3;
            Scalar savedValue [numdataonly];
            Index j = 0;
            int flag = 0;
            Scalar middleValue [numdataonly];

            Index outIdx = m_data.m_LocationList[ValidCellIndex];
            for (Index i = Cellbegin; i < Cellend; i++) {

               const Index c1 = m_data.m_cl[i];
               const Index c2 = m_data.m_cl[i+1];
               if (c1 == sidebegin) { // Wenn die Seite endet

                  sidebegin = -1;
                  if (vertexSaved) {
                     m_data.m_outputpointer[0][outIdx] = savedVertex[0];
                     m_data.m_outputpointer[1][outIdx] = savedVertex[1];
                     m_data.m_outputpointer[2][outIdx] = savedVertex[2];

                     for(Index i = 0; i < numdataonly; i++){
                     m_data.m_outputpointer[i][outIdx] = savedValue[i];
                     };

                     outIdx += 2;
                     vertexSaved=false;
                  }
                  continue;
               } else if(sidebegin == -1) { //Wenn die Neue Seite beginnt

                  flag = 0;
                  sidebegin = c1;
               }

               Scalar cd1 [numdataonly];
               Scalar cd2 [numdataonly];

               for(int i = 0; i < numdataonly; i++){
                   cd1[i] = m_data.m_inputpointer[i+3][c1];
                   cd2[i] = m_data.m_inputpointer[i+3][c2];
               }

               Scalar d1 = m_data.m_inputpointer[3][c1];
               Scalar d2 = m_data.m_inputpointer[3][c2];


               Vector v1(m_data.m_inputpointer[0][c1], m_data.m_inputpointer[1][c1], m_data.m_inputpointer[2][c1]);
               Vector v2(m_data.m_inputpointer[0][c2], m_data.m_inputpointer[1][c2], m_data.m_inputpointer[2][c2]);

               if (d1 <= m_data.m_isovalue && d2 > m_data.m_isovalue) {

                  Vector v = interp(m_data.m_isovalue, v1, v2, d1, d2);
                  Scalar cv[numdataonly];
                         for(Index i = 0; i < numdataonly; i++){
                          cv[i] = cvinterp(m_data.m_isovalue, d1, d2, cd1[i], cd2[i]);
                  };

                         std::cerr << "m_iso " << m_data.m_isovalue << std::endl;
                         std::cerr << "d1 " << d1 << std::endl;
                         std::cerr << "d2 " << d2 << std::endl;
                         std::cerr << "v1 " << v1 << std::endl;
                         std::cerr << "v2 " << v2 << std::endl;
                         std::cerr << "v  " << interp(m_data.m_isovalue, v1, v2, d1, d2) << std::endl;

                  for(Index i = 0; i < numdataonly; i++){

                      middleValue[i] += cv[i];

                  }

                  m_data.m_outputpointer[0][outIdx] = v[0];
                  m_data.m_outputpointer[1][outIdx] = v[1];
                  m_data.m_outputpointer[2][outIdx] = v[2];

                  for(Index i = 0; i < numdataonly; i++){
                  m_data.m_outputpointer[i][outIdx] = cv[i];
                  }

                  ++outIdx;
                  ++j;
                  flag = 1;

               } else if (d1 > m_data.m_isovalue && d2 <= m_data.m_isovalue) {

                  Vector v = interp(m_data.m_isovalue, v1, v2, d1, d2);

                  Scalar cv [numdataonly];
                         for(Index i = 0; i < numdataonly; i++){
                          cv[i] = cvinterp(m_data.m_isovalue, d1, d2, cd1[i], cd2[i]);
                  };

                         middleVector += v;

                  for(Index i = 0; i < numdataonly; i++){

                      middleValue[i] += cv[i];

                  };
                  ++j;
                  if (flag == 1) { //fall 2 nach fall 1

                      m_data.m_outputpointer[0][outIdx] = v[0];
                      m_data.m_outputpointer[1][outIdx] = v[1];
                      m_data.m_outputpointer[2][outIdx] = v[2];

                      for(Index i = 0; i < numdataonly; i++){
                      m_data.m_outputpointer[i][outIdx] = cv[i];

                      }

                     outIdx+=2;

                  } else { //fall 2 zuerst

                     savedVertex = v;

                     for(Index i = 0; i < numdataonly; i++){
                     savedValue[i] = cv[i];
                     }
                     vertexSaved=true;
                  }
               }
            }
            middleVector /= j;


            for (Index i = 2; i < numVert; i+=3) {

               const Index idx = m_data.m_LocationList[ValidCellIndex]+i;

               m_data.m_outputpointer[0][idx] = middleVector[0];
               m_data.m_outputpointer[1][idx] = middleVector[1];
               m_data.m_outputpointer[2][idx] = middleVector[2];

               for(Index a = 0; a < numdataonly; a++){

                   middleValue[a] /= j;
                   m_data.m_outputpointer[3][idx] = middleValue[a];

               }
            }
         }
         break;
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

         float val = m_data.m_volumedata[m_data.m_cl[i]];
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

            tableIndex += (((int) (m_data.m_volumedata[m_data.m_cl[Start+idx]] > m_data.m_isovalue)) << idx);
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

            SIndex sidebegin = -1;
            Index vertcounter = 0;
            for (Index i = Start; i < Start + diff; i++) {

               if (m_data.m_cl[i] == sidebegin) {
                  sidebegin = -1;
                  continue;
               }

               if (sidebegin == -1) {
                  sidebegin = m_data.m_cl[i];
               }

               if (m_data.m_volumedata[m_data.m_cl[i]] <= m_data.m_isovalue && m_data.m_volumedata[m_data.m_cl[i+1]] > m_data.m_isovalue) {

                  vertcounter += 1;
               } else if(m_data.m_volumedata[m_data.m_cl[i]] > m_data.m_isovalue && m_data.m_volumedata[m_data.m_cl[i+1]] <= m_data.m_isovalue) {

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

IsoSurface::IsoSurface(const std::string &shmname, int rank, int size, int moduleID)
: Module("IsoSurface", shmname, rank, size, moduleID) {


   setDefaultCacheMode(ObjectCache::CacheAll);
   setReducePolicy(message::ReducePolicy::OverAll);

   createInputPort("grid_in");
   createInputPort("data_in");
   createInputPort("mapdata_in");

   createOutputPort("grid_out");
   createOutputPort("mapdata_out");

   m_processortype = addIntParameter("processortype", "processortype", 0, Parameter::Choice);
   std::vector<std::string> choices;
   choices.push_back("OpenMP");
   choices.push_back("CUDA");
   setParameterChoices(m_processortype, choices);

   m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
   m_shader = addStringParameter("shader", "name of shader to apply to geometry", "");
   m_shaderParams = addStringParameter("shader_params", "shader parameters (as \"key=value\" \"key=value1 value2\"", "");
}

IsoSurface::~IsoSurface() {

}

bool IsoSurface::prepare() {

   min = std::numeric_limits<Scalar>::max() ;
   max = -std::numeric_limits<Scalar>::max();
   return Module::prepare();
}

bool IsoSurface::reduce(int timestep) {

   boost::mpi::all_reduce(boost::mpi::communicator(),
         min, min, boost::mpi::minimum<Scalar>());
   boost::mpi::all_reduce(boost::mpi::communicator(),
         max, max, boost::mpi::maximum<Scalar>());

   setParameterRange(m_isovalue, (double)min, (double)max);

   return Module::reduce(timestep);
}

class Leveller {

   UnstructuredGrid::const_ptr m_grid;
   std::vector<Object::const_ptr> m_data;
   std::vector<Object::const_ptr> m_mapdata;
   Scalar m_isoValue;
   Index m_processortype;

   unsigned char *tl;
   Index *el;
   Index *cl;
   const Scalar *x;
   const Scalar *y;
   const Scalar *z;

   const Scalar *d;
   const Scalar *md;

   Index *out_cl;
   Scalar *out_x;
   Scalar *out_y;
   Scalar *out_z;
   Scalar *out_d;

   Triangles::ptr m_triangles;
   Vec<Scalar>::ptr m_outData;
   Vec<Scalar>::ptr m_outmapData;

   Scalar gmin, gmax;

 public:
   Leveller(UnstructuredGrid::const_ptr grid, const Scalar isovalue, Index processortype)
      : m_grid(grid)
      , m_isoValue(isovalue)
      , m_processortype(processortype)
      , tl(nullptr)
      , el(nullptr)
      , cl(nullptr)
      , x(nullptr)
      , y(nullptr)
      , z(nullptr)
      , d(nullptr)
      , out_cl(nullptr)
      , out_x(nullptr)
      , out_y(nullptr)
      , out_z(nullptr)
      , out_d(nullptr)
      , gmin(std::numeric_limits<Scalar>::max())
      , gmax(-std::numeric_limits<Scalar>::max())
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

   bool process(Index inputdim) {

      Vec<Scalar>::const_ptr mapdataobj = Vec<Scalar>::as(m_mapdata[0]);

      Vec<Scalar>::const_ptr dataobj = Vec<Scalar>::as(m_data[0]);
      if (!dataobj)
         return false;

      Index totalNumVertices = 0;

      switch (m_processortype) {

         case 0: {

            HostData HD(m_isoValue, dataobj->x(), mapdataobj->x(), m_grid->el(), m_grid->tl(), m_grid->cl(), m_grid->x(), m_grid->y(), m_grid->z(), inputdim);
#if defined(USE_OMP)
            totalNumVertices = calculateSurface<HostData, decltype(thrust::omp::par)>(HD);
#elif defined(USE_TBB)
            totalNumVertices = calculateSurface<HostData, decltype(thrust::tbb::par)>(HD);
#else
            totalNumVertices = calculateSurface<HostData, decltype(thrust::cpp::par)>(HD);
#endif

//            m_triangles->d()->x[0] = HD.m_xCoordinateVector;
//            m_triangles->d()->x[1] = HD.m_yCoordinateVector;
//            m_triangles->d()->x[2] = HD.m_zCoordinateVector;
//            m_outmapData->d()->x[0] = HD.m_mapdataVector;


            m_triangles->d()->x[0] = HD.m_outData[0];
            m_triangles->d()->x[1] = HD.m_outData[1];
            m_triangles->d()->x[2] = HD.m_outData[2];
            m_outmapData->d()->x[0] = HD.m_outData[3];

      }
         break;

         case 1: {

            DeviceData DD(m_isoValue, dataobj->x(), mapdataobj->x(), m_grid->el(), m_grid->tl(), m_grid->cl(), m_grid->x(), m_grid->y(), m_grid->z());
            // totalNumVertices = calculateSurface<DeviceData, decltype(thrust::cuda::par)>(DD);

            m_triangles->x().resize(totalNumVertices);
            out_x = m_triangles->x().data();
            m_triangles->y().resize(totalNumVertices);
            out_y = m_triangles->y().data();
            m_triangles->z().resize(totalNumVertices);
            out_z = m_triangles->z().data();

            thrust::copy(DD.m_xCoordinateVector.begin(), DD.m_xCoordinateVector.end(), out_x);
            thrust::copy(DD.m_yCoordinateVector.begin(), DD.m_yCoordinateVector.end(), out_y);
            thrust::copy(DD.m_zCoordinateVector.begin(), DD.m_zCoordinateVector.end(), out_z);
         }
         break;
      }

      m_triangles->cl().resize(totalNumVertices);
      out_cl = m_triangles->cl().data();
      thrust::counting_iterator<Index> first(0), last(totalNumVertices);
      thrust::copy(first, last, out_cl);

      return true;
   }

   template<class Data, class pol>
   Index calculateSurface(Data &data) {

         thrust::counting_iterator<int> first(0);
         thrust::counting_iterator<int> last = first + m_grid->getNumElements();

         typedef thrust::tuple<typename Data::Indexiterator, typename Data::Indexiterator> Iteratortuple;
         typedef thrust::zip_iterator<Iteratortuple> ZipIterator;

         ZipIterator ElTupleVec(thrust::make_tuple(data.m_el.begin(), data.m_el.begin()+1));

         data.m_ValidCellVector.resize(m_grid->getNumElements());

         auto end = thrust::copy_if(pol(), first, last, ElTupleVec, data.m_ValidCellVector.begin(), checkcell<Data>(data));

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

         data.m_xCoordinateVector->resize(totalNumVertices);
         data.m_yCoordinateVector->resize(totalNumVertices);
         data.m_zCoordinateVector->resize(totalNumVertices);
         data.m_mapdataVector->resize(totalNumVertices);

         for(int i = 0; i < data.m_numinputdata; i++){

         data.m_outData[i]->resize(totalNumVertices);

         };

         thrust::counting_iterator<Index> start(0), finish(numValidCells);
         thrust::for_each(pol(), start, finish, process_Cell<Data>(data));

         return totalNumVertices;
      }

   Index addData(Object::const_ptr obj, Object::const_ptr mapobj) {

      Index m_inputdim = 3;

      m_data.push_back(obj);
      auto data = Vec<Scalar, 1>::as(obj);
      if (data) {
         d = &data->x()[0];
         m_outData = Vec<Scalar>::ptr(new Vec<Scalar>(Object::Initialized));
         m_outData->setMeta(data->meta());

      }
      m_mapdata.push_back(mapobj);
      auto mapdata = Vec<Scalar, 1>::as(mapobj);
      if (mapdata) {
         md = &mapdata->x()[0];
         m_outmapData = Vec<Scalar>::ptr(new Vec<Scalar>(Object::Initialized));
         m_outmapData->setMeta(mapdata->meta());
         m_inputdim++;
      }
      auto vecdata = Vec<Scalar,3>::as(mapobj);
      if(vecdata) {
          md = &mapdata->x()[0];
          m_outmapData = Vec<Scalar>::ptr(new Vec<Scalar>(Object::Initialized));
          m_outmapData->setMeta(mapdata->meta());
          m_inputdim=+3;

      }
      return m_inputdim;
   }

   Object::ptr result() {

      return m_triangles;
   }

   Object::ptr mapresult() {

      return m_outmapData;
   }

   std::pair<Scalar, Scalar> range() {

      return std::make_pair(gmin, gmax);
   }
};

std::pair<Object::ptr, Object::ptr>
IsoSurface::generateIsoSurface(Object::const_ptr grid_object,
      Object::const_ptr data_object,
      Object::const_ptr mapdata_object,
      const Scalar isoValue,
      int processorType) {
    Object::ptr result;
    Object::ptr mapresult;
    Index inputdim;

   if (!grid_object || !data_object)
      return std::make_pair(result, mapresult);

   UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(grid_object);
   Vec<Scalar>::const_ptr data = Vec<Scalar>::as(data_object);
   Vec<Scalar>::const_ptr mapdata = Vec<Scalar>::as(mapdata_object);

   if (!grid || !data) {
      std::cerr << "IsoSurface: incompatible input" << std::endl;
      return std::make_pair(result, mapresult);
   }

   Leveller l(grid, isoValue, processorType);

   inputdim = l.addData(data, mapdata);

   l.process(inputdim);

   auto range = l.range();
   if (range.first < min)
      min = range.first;
   if (range.second > max)
      max = range.second;

   result = l.result();
   mapresult = l.mapresult();

   return std::make_pair(result, mapresult);

}

bool IsoSurface::compute() {

   const Scalar isoValue = getFloatParameter("isovalue");

   std::cerr<< " ISO " << isoValue << std::endl;

   Index processorType = getIntParameter("processortype");

   while (hasObject("grid_in") && hasObject("data_in")) {

      Object::const_ptr grid = takeFirstObject("grid_in");
      Object::const_ptr data = takeFirstObject("data_in");
      Object::const_ptr mapdata = takeFirstObject("mapdata_in");
      //calc number of objectdimensions here

      std::pair<Object::ptr, Object::ptr> object = generateIsoSurface(grid, data, mapdata, isoValue, processorType);

      if (object.first && !object.first->isEmpty()) {
         object.first->copyAttributes(data);
         object.second->copyAttributes(mapdata);
         object.first->copyAttributes(grid, false);
         if (!m_shader->getValue().empty()) {
            object.first->addAttribute("shader", m_shader->getValue());
            if (!m_shaderParams->getValue().empty()) {
               object.first->addAttribute("shader_params", m_shaderParams->getValue());
            }
         }
         addObject("grid_out", object.first);
         addObject("mapdata_out", object.second);
      }
   }

   return true;
}
