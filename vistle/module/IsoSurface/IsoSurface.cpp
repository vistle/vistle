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

#include <thrust/system/omp/vector.h>
#include <thrust/system/omp/execution_policy.h>
#include <util/openmp.h>

MODULE_MAIN(IsoSurface)

using namespace vistle;


#define lerp(a, b, t) ( a + t * (b - a) )

const Scalar EPSILON = 1.0e-10f;

inline Vector lerp3(const Vector &a, const Vector &b, const Scalar t) {

    return lerp(a, b, t);
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

struct HostData {

    Scalar m_isovalue;
    const vistle::shm<Scalar>::array &m_volumedata;
    const vistle::shm<Index>::array &m_cl;
    const vistle::shm<Index>::array &m_el;
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

    Scalar* m_xpointer;
    Scalar* m_ypointer;
    Scalar* m_zpointer;
    typedef vistle::shm<Index>::array::iterator Indexiterator;

    HostData(Scalar isoValue
             , const vistle::shm<Scalar>::array &data
             , const vistle::shm<Index>::array &el
             , const vistle::shm<unsigned char>::array &tl
             , const vistle::shm<Index>::array &cl
             , const vistle::shm<Scalar>::array &x
             , const vistle::shm<Scalar>::array &y
             , const vistle::shm<Scalar>::array &z)

        : m_isovalue(isoValue), m_volumedata(data), m_el(el), m_cl(cl), m_tl(tl), m_x(x), m_y(y), m_z(z) {

        m_xCoordinateVector = new vistle::ShmVector<Scalar>;
        m_yCoordinateVector = new vistle::ShmVector<Scalar>;
        m_zCoordinateVector = new vistle::ShmVector<Scalar>;

    }


    std::vector<Scalar> xCoordinateVector;
    std::vector<Scalar> yCoordinateVector;
    std::vector<Scalar> zCoordinateVector;


};

struct DeviceData {
    Scalar m_isovalue;
    thrust::device_vector<Scalar> m_volumedata;
    thrust::device_vector<Index> m_cl;
    thrust::device_vector<Index> m_el;
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
    typedef thrust::device_vector<Index>::iterator Indexiterator;

    DeviceData(Scalar isoValue
               , const vistle::shm<Scalar>::array &data
               , const vistle::shm<Index>::array &el
               , const vistle::shm<unsigned char>::array &tl
               , const vistle::shm<Index>::array &cl
               , const vistle::shm<Scalar>::array &x
               , const vistle::shm<Scalar>::array &y
               , const vistle::shm<Scalar>::array &z)

        : m_isovalue(isoValue) {

        m_volumedata.resize(data.size());
        for(Index i = 0; i < data.size(); i++){
            m_volumedata[i] = data[i];
        }
        m_el.resize(el.size());
        for(Index i = 0; i < el.size(); i++){
            m_el[i]=el[i];
        }
        m_cl.resize(cl.size());
        for(Index i = 0; i < cl.size(); i++){
            m_cl[i]=cl[i];
        }
        m_tl.resize(tl.size());
        for(Index i = 0; i < tl.size(); i++){
            m_tl[i] = tl[i];
        }
        m_x.resize(x.size());
        for(Index i = 0; i < x.size(); i++){
            m_x[i] = x[i];
        }
        m_y.resize(y.size());
        for(Index i = 0; i < y.size(); i++){
            m_y[i] = y[i];
        }
        m_z.resize(z.size());
        for(Index i = 0; i < z.size(); i++){
            m_z[i] = z[i];
        }
    }

};

template<class Data>

struct process_Cell {
    process_Cell(Data &data) : m_data(data) {

        Index totalnumvertices = 0;

        if(!m_data.m_numVertices.empty()){

            totalnumvertices += m_data.m_numVertices.back();

        }

        if(!m_data.m_LocationList.empty()){

            totalnumvertices += m_data.m_LocationList.back();

        }

        m_data.m_xCoordinateVector->resize(totalnumvertices);
        m_data.m_yCoordinateVector->resize(totalnumvertices);
        m_data.m_zCoordinateVector->resize(totalnumvertices);

        m_data.m_xpointer = m_data.m_xCoordinateVector->data();
        m_data.m_ypointer = m_data.m_yCoordinateVector->data();
        m_data.m_zpointer = m_data.m_zCoordinateVector->data();

    }

    Data &m_data;

    __host__ __device__
    int operator()(Index ValidCellIndex)

    {
        Index CellNr = m_data.m_ValidCellVector[ValidCellIndex];

        Index Cellbegin = m_data.m_el[CellNr];

        Index Cellend = m_data.m_el[CellNr+1];

        switch(m_data.m_tl[CellNr]){

        case UnstructuredGrid::HEXAHEDRON:{

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

            Vector newv[8];

            for (int idx = 0; idx < 8; idx ++) {
                newv[idx][0] = m_data.m_x[newindex[idx]];
                newv[idx][1] = m_data.m_y[newindex[idx]];
                newv[idx][2] = m_data.m_z[newindex[idx]];
            }

            Vector vertlist[12];

            vertlist[0] = interp(m_data.m_isovalue, newv[0], newv[1], field[0], field[1]);
            vertlist[1] = interp(m_data.m_isovalue, newv[1], newv[2], field[1], field[2]);
            vertlist[2] = interp(m_data.m_isovalue, newv[2], newv[3], field[2], field[3]);
            vertlist[3] = interp(m_data.m_isovalue, newv[3], newv[0], field[3], field[0]);

            vertlist[4] = interp(m_data.m_isovalue, newv[4], newv[5], field[4], field[5]);
            vertlist[5] = interp(m_data.m_isovalue, newv[5], newv[6], field[5], field[6]);
            vertlist[6] = interp(m_data.m_isovalue, newv[6], newv[7], field[6], field[7]);
            vertlist[7] = interp(m_data.m_isovalue, newv[7], newv[4], field[7], field[4]);

            vertlist[8] = interp(m_data.m_isovalue, newv[0], newv[4], field[0], field[4]);
            vertlist[9] = interp(m_data.m_isovalue, newv[1], newv[5], field[1], field[5]);
            vertlist[10] = interp(m_data.m_isovalue, newv[2], newv[6], field[2], field[6]);
            vertlist[11] = interp(m_data.m_isovalue, newv[3], newv[7], field[3], field[7]);

            for (int idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx += 3) {

                for (int i=0; i<3; ++i) {

                    int edge = hexaTriTable[m_data.m_caseNums[ValidCellIndex]][idx+i];

                    Vector &newv = vertlist[edge];

                    m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[0];
                    m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[1];
                    m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[2];

                }
            }
        }
            break;

        case UnstructuredGrid::TETRAHEDRON:{

            Index newindex[4];

            newindex[0] = m_data.m_cl[Cellbegin ];
            newindex[1] = m_data.m_cl[Cellbegin +1];
            newindex[2] = m_data.m_cl[Cellbegin +2];
            newindex[3] = m_data.m_cl[Cellbegin +3];

            Scalar field[4];
            for (int idx = 0; idx < 4; idx ++) {
                field[idx] = m_data.m_volumedata[newindex[idx]];
            }

            Vector newv[4];

            for (int idx = 0; idx < 4; idx ++) {
                newv[idx][0] = m_data.m_x[newindex[idx]];
                newv[idx][1] = m_data.m_y[newindex[idx]];
                newv[idx][2] = m_data.m_z[newindex[idx]];
            }

            Vector vertlist[6];

            vertlist[0] = interp(m_data.m_isovalue, newv[0], newv[1], field[0], field[1]);
            vertlist[1] = interp(m_data.m_isovalue, newv[0], newv[2], field[0], field[2]);

            vertlist[2] = interp(m_data.m_isovalue, newv[0], newv[3], field[0], field[3]);

            vertlist[3] = interp(m_data.m_isovalue, newv[1], newv[2], field[1], field[2]);
            vertlist[4] = interp(m_data.m_isovalue, newv[1], newv[3], field[1], field[3]);
            vertlist[5] = interp(m_data.m_isovalue, newv[2], newv[3], field[2], field[3]);

            for (int idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx += 3) {

                for (int i=0; i<3; ++i) {

                    int edge = tetraTriTable[m_data.m_caseNums[ValidCellIndex]][idx+i];

                    Vector &newv = vertlist[edge];

                    m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[0];
                    m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[1];
                    m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[2];

                }
            }
        }
            break;

        case UnstructuredGrid::PYRAMID:{

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

            Vector newv[5];

            for (int idx = 0; idx < 5; idx ++) {
                newv[idx][0] = m_data.m_x[newindex[idx]];
                newv[idx][1] = m_data.m_y[newindex[idx]];
                newv[idx][2] = m_data.m_z[newindex[idx]];
            }

            Vector vertlist[8];

            vertlist[0] = interp(m_data.m_isovalue, newv[0], newv[1], field[0], field[1]);
            vertlist[1] = interp(m_data.m_isovalue, newv[0], newv[4], field[0], field[4]);

            vertlist[2] = interp(m_data.m_isovalue, newv[0], newv[3], field[0], field[3]);
            vertlist[3] = interp(m_data.m_isovalue, newv[1], newv[4], field[1], field[4]);

            vertlist[4] = interp(m_data.m_isovalue, newv[1], newv[2], field[1], field[2]);
            vertlist[5] = interp(m_data.m_isovalue, newv[2], newv[3], field[2], field[3]);

            vertlist[6] = interp(m_data.m_isovalue, newv[2], newv[4], field[2], field[4]);
            vertlist[7] = interp(m_data.m_isovalue, newv[3], newv[4], field[3], field[4]);

            for (int idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx += 3) {

                for (int i=0; i<3; ++i) {

                    int edge = pyrTriTable[m_data.m_caseNums[ValidCellIndex]][idx+i];

                    Vector &newv = vertlist[edge];

                    m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[0];
                    m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[1];
                    m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[2];

                }
            }
        }
            break;

        case UnstructuredGrid::PRISM:{

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

            Vector newv[6];

            for (int idx = 0; idx < 6; idx ++) {
                newv[idx][0] = m_data.m_x[newindex[idx]];
                newv[idx][1] = m_data.m_y[newindex[idx]];
                newv[idx][2] = m_data.m_z[newindex[idx]];
            }

            Vector vertlist[9];

            vertlist[0] = interp(m_data.m_isovalue, newv[0], newv[1], field[0], field[1]);
            vertlist[1] = interp(m_data.m_isovalue, newv[1], newv[2], field[1], field[2]);
            vertlist[2] = interp(m_data.m_isovalue, newv[0], newv[2], field[0], field[2]);

            vertlist[3] = interp(m_data.m_isovalue, newv[3], newv[4], field[3], field[4]);
            vertlist[4] = interp(m_data.m_isovalue, newv[4], newv[5], field[4], field[5]);
            vertlist[5] = interp(m_data.m_isovalue, newv[3], newv[5], field[3], field[5]);

            vertlist[6] = interp(m_data.m_isovalue, newv[0], newv[3], field[0], field[3]);
            vertlist[7] = interp(m_data.m_isovalue, newv[1], newv[4], field[1], field[4]);
            vertlist[8] = interp(m_data.m_isovalue, newv[2], newv[5], field[2], field[5]);

            for (int idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx += 3) {

                for (int i=0; i<3; ++i) {

                    int edge = prismTriTable[m_data.m_caseNums[ValidCellIndex]][idx+i];

                    Vector &newv = vertlist[edge];

                    m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[0];
                    m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[1];
                    m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+idx+i] = newv[2];
                }
            }
        }
            break;

        case UnstructuredGrid::POLYHEDRON: {
            Vector newv[Cellend-Cellbegin];
            Vector interpolationValues[CellNr];
            int sidebegin = -1;
            int j = 0;

            int flag = 0;

            for (int idx = Cellbegin; idx < Cellend; idx ++) {

                newv[idx-Cellbegin][0] = m_data.m_x[m_data.m_cl[idx]];
                newv[idx-Cellbegin][1] = m_data.m_y[m_data.m_cl[idx]];
                newv[idx-Cellbegin][2] = m_data.m_z[m_data.m_cl[idx]];

            }

            bool vertexSaved=false;
            Vector savedVertex;
            for(int i = Cellbegin; i < Cellend; i++) {

                if (m_data.m_cl[i] == sidebegin){// Wenn die Seite endet

                    sidebegin = -1;
                    if (vertexSaved) {
                        interpolationValues[j] = savedVertex;
                        ++j;
                        vertexSaved=false;
                    }
                    continue;
                } else if(sidebegin == -1) { //Wenn die Neue Seite beginnt

                    flag = 0;
                    sidebegin = m_data.m_cl[i];

                }

                if(m_data.m_volumedata[m_data.m_cl[i]] <= m_data.m_isovalue && m_data.m_volumedata[m_data.m_cl[i+1]] > m_data.m_isovalue){
                    interpolationValues[j] = interp(m_data.m_isovalue, newv[i-Cellbegin], newv[i+1-Cellbegin],m_data.m_volumedata[m_data.m_cl[i]], m_data.m_volumedata[m_data.m_cl[i+1]] );
                    ++j;
                    flag = 1;


                } else if(m_data.m_volumedata[m_data.m_cl[i]] > m_data.m_isovalue && m_data.m_volumedata[m_data.m_cl[i+1]] <= m_data.m_isovalue){
                    if(flag == 1){//fall 2 nach fall 1

                        interpolationValues[j] = interp(m_data.m_isovalue, newv[i-Cellbegin], newv[i+1-Cellbegin], m_data.m_volumedata[m_data.m_cl[i]], m_data.m_volumedata[m_data.m_cl[i+1]] );
                        ++j;
                    } else {//fall 2 zuerst

                        savedVertex= interp(m_data.m_isovalue, newv[i-Cellbegin], newv[i+1-Cellbegin], m_data.m_volumedata[m_data.m_cl[i]], m_data.m_volumedata[m_data.m_cl[i+1]]);
                        vertexSaved=true;
                    }

                }

            }

            Vector middleVector(0,0,0);

            for(int i = 0; i < j; i++){

                middleVector += interpolationValues[i];
            };

            middleVector = middleVector/j;

            int k = 0;

            for (int i = 0; i < m_data.m_numVertices[ValidCellIndex]; i+=3){


                m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+i] = interpolationValues[k][0];
                m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+i] = interpolationValues[k][1];
                m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+i] = interpolationValues[k][2];

                m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+i+1] = interpolationValues[k+1][0];
                m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+i+1] = interpolationValues[k+1][1];
                m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+i+1] = interpolationValues[k+1][2];

                m_data.m_xpointer[m_data.m_LocationList[ValidCellIndex]+i+2] = middleVector[0];
                m_data.m_ypointer[m_data.m_LocationList[ValidCellIndex]+i+2] = middleVector[1];
                m_data.m_zpointer[m_data.m_LocationList[ValidCellIndex]+i+2] = middleVector[2];

                k+=2;

            }
        }
        }
    }
};

template<class Data>

struct checkcell {
    checkcell(Data &data) : m_data(data) {}

    Data &m_data;

    typedef float argument_type;

    typedef float result_type;

    __host__ __device__ int operator()(const thrust::tuple<Index,Index> iCell) const {


        int havelower = 0;
        int havehigher = 0;

        Index Cell = iCell.get<0>();
        Index nextCell = iCell.get<1>();

        for (Index i=Cell; i<nextCell; i++) {

            float val = m_data.m_volumedata[m_data.m_cl[i]];
            if (val>m_data.m_isovalue) {
                havelower=1;
                if(havehigher)
                    return 1;
            }else {
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

    __host__ __device__
    thrust::tuple<Index,Index> operator()(Index CellNr )
    {
        uint tableIndex = 0;
        Index Start = m_data.m_el[CellNr];
        Index diff = m_data.m_el[CellNr+1]-Start;
        unsigned char CellType = m_data.m_tl[CellNr];
        int numVerts = 0;


        if(CellType != UnstructuredGrid::POLYHEDRON){
            for (int idx = 0; idx < diff; idx ++){

                tableIndex += (((int) (m_data.m_volumedata[m_data.m_cl[Start+idx]] > m_data.m_isovalue)) << idx);
            }
        }
        switch(CellType){

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

        case UnstructuredGrid::POLYHEDRON:{

            int sidebegin = -1;
            int vertcounter = 0;

            for(Index i = Start; i < Start + diff; i++){

                if (m_data.m_cl[i] == sidebegin){

                    sidebegin = -1;

                    continue;
                }

                if(sidebegin == -1){

                    sidebegin = m_data.m_cl[i];

                }

                if(m_data.m_volumedata[m_data.m_cl[i]] <= m_data.m_isovalue && m_data.m_volumedata[m_data.m_cl[i+1]] > m_data.m_isovalue){

                    vertcounter += 1;

                }else if(m_data.m_volumedata[m_data.m_cl[i]] > m_data.m_isovalue && m_data.m_volumedata[m_data.m_cl[i+1]] <= m_data.m_isovalue){

                    vertcounter += 1;

                }

            }

            numVerts = vertcounter + vertcounter/2;
        }
            break;

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

    createOutputPort("grid_out");

    m_processortype = addIntParameter("processortype", "processortype", 0, Parameter::Choice);

    std::vector<std::string> choices;

    choices.push_back("OpenMP");
    choices.push_back("CUDA");

    setParameterChoices(m_processortype, choices);

    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
    m_shader = addStringParameter("shader", "name of shader to apply to geometry", "Gouraud");
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
    Scalar m_isoValue;
    Index m_processortype;

    unsigned char *tl;
    Index *el;
    Index *cl;
    const Scalar *x;
    const Scalar *y;
    const Scalar *z;

    const Scalar *d;

    Index *out_cl;
    Scalar *out_x;
    Scalar *out_y;
    Scalar *out_z;
    Scalar *out_d;

    Triangles::ptr m_triangles;
    Vec<Scalar>::ptr m_outData;

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

    bool run(){

        Vec<Scalar>::const_ptr dataobj = Vec<Scalar>::as(m_data[0]);
        if(!dataobj){
            return false;
        }

        Index totalNumVertices = 0;

        switch(m_processortype){



        case 0:{

            HostData HD(m_isoValue, dataobj->x(), m_grid->el(), m_grid->tl(), m_grid->cl(), m_grid->x(), m_grid->y(), m_grid->z());

            calculateSurface<HostData, decltype(thrust::omp::par)>(HD);

            if(!HD.m_numVertices.empty()){

               totalNumVertices += HD.m_numVertices.back();

            }

            if(!HD.m_LocationList.empty()){

               totalNumVertices += HD.m_LocationList.back();

            }

            m_triangles->d()->x[0] = HD.m_xCoordinateVector;
            m_triangles->d()->x[1] = HD.m_yCoordinateVector;
            m_triangles->d()->x[2] = HD.m_zCoordinateVector;

        } break;

        case 1:{

            DeviceData DD(m_isoValue, dataobj->x(), m_grid->el(), m_grid->tl(), m_grid->cl(), m_grid->x(), m_grid->y(), m_grid->z());

//            calculateSurface<DeviceData, decltype(thrust::cuda::par)>(DD);

            if(!DD.m_numVertices.empty()){

               totalNumVertices += DD.m_numVertices.back();

            }

            if(!DD.m_LocationList.empty()){

               totalNumVertices += DD.m_LocationList.back();

            }

            m_triangles->x().resize(totalNumVertices);
            out_x = m_triangles->x().data();
            m_triangles->y().resize(totalNumVertices);
            out_y = m_triangles->y().data();
            m_triangles->z().resize(totalNumVertices);
            out_z = m_triangles->z().data();

            thrust::copy(DD.m_xCoordinateVector.begin(), DD.m_xCoordinateVector.end(), out_x);
            thrust::copy(DD.m_yCoordinateVector.begin(), DD.m_yCoordinateVector.end(), out_y);
            thrust::copy(DD.m_zCoordinateVector.begin(), DD.m_zCoordinateVector.end(), out_z);

        } break;
        }

        m_triangles->cl().resize(totalNumVertices);
        out_cl = m_triangles->cl().data();
        {
            thrust::counting_iterator<Index> first(0), last(totalNumVertices);
            thrust::copy(first, last, out_cl);
        }

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

        Index totalNumVertices = data.m_numVertices.back() + data.m_LocationList.back();

        thrust::counting_iterator<Index> start(0), finish(numValidCells);

        thrust::for_each(pol(), start, finish, process_Cell<Data>(data));

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
#if 0
        const Index numElem = m_grid->getNumElements();

        Index curidx = 0;
        std::vector<Index> outputIdx(numElem);
#pragma omp parallel
        {
            Scalar tmin = std::numeric_limits<Scalar>::max();
            Scalar tmax = -std::numeric_limits<Scalar>::max();
#pragma omp for schedule (dynamic)
            for (Index elem=0; elem<numElem; ++elem) {

                Index n = 0;
                switch (tl[elem]) {
                case UnstructuredGrid::HEXAHEDRON: {
                    n = processHexahedron(elem, 0, true /* count only */, tmin, tmax);
                }
                }
#pragma omp critical
                {
                    outputIdx[elem] = curidx;
                    curidx += n;
                }
            }
#pragma omp critical
            {
                if (tmin < gmin)
                    gmin = tmin;
                if (tmax > gmax)
                    gmax = tmax;
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
                processHexahedron(elem, outputIdx[elem], false, gmin, gmax);
            }


            }
        }




#endif

        run();

        return true;
    }

    Object::ptr result() {

        return m_triangles;
    }

    std::pair<Scalar, Scalar> range() {

        return std::make_pair(gmin, gmax);
    }

   private:
         Index processHexahedron(const Index elem, const Index outIdx, bool numVertsOnly, Scalar &min, Scalar &max) {

            const Index p = el[elem];

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
            for (int idx = 0; idx < 8; idx ++) {
               field[idx] = d[index[idx]];
            }

            uint tableIndex = 0;
            for (int idx = 0; idx < 8; idx ++)
               tableIndex += (((int) (field[idx] < m_isoValue)) << idx);

            const int numVerts = hexaNumVertsTable[tableIndex];

            if (numVertsOnly) {
               for (int idx = 0; idx < 8; ++idx) {
                  if (field[idx] < min)
                     min = field[idx];
                  if (field[idx] > max)
                     max = field[idx];
               }
               return numVerts;
            }

            if (numVerts > 0) {

               Vector v[8];
               for (int idx = 0; idx < 8; idx ++) {
                  v[idx][0] = x[index[idx]];
                  v[idx][1] = y[index[idx]];
                  v[idx][2] = z[index[idx]];
               }

               Vector vertlist[12];
               vertlist[0] = interp(m_isoValue, v[0], v[1], field[0], field[1]);
               vertlist[1] = interp(m_isoValue, v[1], v[2], field[1], field[2]);
               vertlist[2] = interp(m_isoValue, v[2], v[3], field[2], field[3]);
               vertlist[3] = interp(m_isoValue, v[3], v[0], field[3], field[0]);

               vertlist[8] = interp(m_isoValue, v[0], v[4], field[0], field[4]);
               vertlist[9] = interp(m_isoValue, v[1], v[5], field[1], field[5]);
               vertlist[10] = interp(m_isoValue, v[2], v[6], field[2], field[6]);
               vertlist[11] = interp(m_isoValue, v[3], v[7], field[3], field[7]);

               for (int idx = 0; idx < numVerts; idx += 3) {

                  for (int i=0; i<3; ++i) {
                     int edge = hexaTriTable[tableIndex][idx+i];
                     Vector &v = vertlist[edge];

                     out_x[outIdx+idx+i] = v[0];
                     out_y[outIdx+idx+i] = v[1];
                     out_z[outIdx+idx+i] = v[2];

                     out_cl[outIdx+idx+i] = outIdx+idx+i;
                  }
               }
            }

            return numVerts;
         }
};

Object::ptr
IsoSurface::generateIsoSurface(Object::const_ptr grid_object,
                               Object::const_ptr data_object,
                               const Scalar isoValue,
                               int processorType) {

    if (!grid_object || !data_object)
        return Object::ptr();

    UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(grid_object);
    Vec<Scalar>::const_ptr data = Vec<Scalar>::as(data_object);

    if (!grid || !data) {
        std::cerr << "IsoSurface: incompatible input" << std::endl;
        return Object::ptr();
    }

    Leveller l(grid, isoValue, processorType);
    l.addData(data);
    l.process();

    auto range = l.range();
    if (range.first < min)
        min = range.first;
    if (range.second > max)
        max = range.second;

    return l.result();
}


bool IsoSurface::compute() {

    const Scalar isoValue = getFloatParameter("isovalue");

    Index processorType = getIntParameter("processortype");

    while (hasObject("grid_in") && hasObject("data_in")) {

        Object::const_ptr grid = takeFirstObject("grid_in");
        Object::const_ptr data = takeFirstObject("data_in");
        Object::ptr object =
                generateIsoSurface(grid, data, isoValue, processorType);

        if (object && !object->isEmpty()) {
            object->copyAttributes(data);
            object->copyAttributes(grid, false);
            if (!m_shader->getValue().empty()) {
                object->addAttribute("shader", m_shader->getValue());
                if (!m_shaderParams->getValue().empty()) {
                    object->addAttribute("shader_params", m_shaderParams->getValue());
                }
            }
            addObject("grid_out", object);
        }
    }

    return true;
}
