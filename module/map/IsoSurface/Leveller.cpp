//
//This code is used for both IsoCut and IsoSurface!
//

#include <sstream>
#include <iomanip>
#include <vistle/core/index.h>
#include <vistle/core/scalar.h>
#include <vistle/core/unstr.h>
#include <vistle/core/triangles.h>
#include <vistle/core/lines.h>
#include <vistle/core/shm.h>
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


const int MaxNumData = 6;
const Scalar EPSILON(1e-10);


struct HostData {
    Scalar m_isovalue;
    int m_numInVertData = 0, m_numInVertDataI = 0, m_numInVertDataB = 0;
    int m_numInCellData = 0, m_numInCellDataI = 0, m_numInCellDataB = 0;
    IsoDataFunctor m_isoFunc;
    const Index *m_el = nullptr;
    const Index *m_cl = nullptr;
    const Byte *m_tl = nullptr;
    std::vector<Index> m_caseNums;
    std::vector<Index> m_numVertices;
    std::vector<Index> m_LocationList;
    std::vector<Index> m_SelectedCellVector;
    bool m_SelectedCellVectorValid = false;
    int m_numVertPerCell = 0;
    Index m_nvert[3];
    Index m_nghost[3][2];
    std::vector<vistle::shm_array_ref<vistle::shm_array<Scalar, shm<Scalar>::allocator>>> m_outVertData, m_outCellData;
    std::vector<vistle::shm_array_ref<vistle::shm_array<Index, shm<Index>::allocator>>> m_outVertDataI, m_outCellDataI;
    std::vector<vistle::shm_array_ref<vistle::shm_array<Byte, shm<Byte>::allocator>>> m_outVertDataB, m_outCellDataB;
    std::vector<const Scalar *> m_inVertPtr, m_inCellPtr;
    std::vector<const Index *> m_inVertPtrI, m_inCellPtrI;
    std::vector<const Byte *> m_inVertPtrB, m_inCellPtrB;
    std::vector<Scalar *> m_outVertPtr, m_outCellPtr;
    std::vector<Index *> m_outVertPtrI, m_outCellPtrI;
    std::vector<Byte *> m_outVertPtrB, m_outCellPtrB;
    bool m_isUnstructured = false;
    bool m_isPoly = false;
    bool m_isTri = false;
    bool m_isQuad = false;
    bool m_haveCoords = false;
    bool m_computeNormals = false;
    bool m_computeGhosts = false;

    typedef const Byte *TypeIterator;
    typedef const Index *IndexIterator;
    typedef std::vector<Index>::iterator VectorIndexIterator;

    // for unstructured grids
    HostData(Scalar isoValue, IsoDataFunctor isoFunc, const Index *el, const Byte *tl, const Byte *ghost,
             const Index *cl, const Scalar *x, const Scalar *y, const Scalar *z)
    : m_isovalue(isoValue)
    , m_isoFunc(isoFunc)
    , m_el(el)
    , m_cl(cl)
    , m_tl(tl)
    , m_nvert{0, 0, 0}
    , m_nghost{{0, 0}, {0, 0}, {0, 0}}
    , m_isUnstructured(el)
    , m_haveCoords(true)
    , m_computeNormals(false)
    , m_computeGhosts(ghost != nullptr)
    {
        addmappeddata(x);
        addmappeddata(y);
        addmappeddata(z);

        if (m_computeGhosts)
            addcelldata(ghost);
    }

    // for structured grids
    HostData(Scalar isoValue, IsoDataFunctor isoFunc, Index nx, Index ny, Index nz, const Scalar *x, const Scalar *y,
             const Scalar *z, bool computeGhost)
    : m_isovalue(isoValue)
    , m_isoFunc(isoFunc)
    , m_el(nullptr)
    , m_cl(nullptr)
    , m_tl(nullptr)
    , m_nvert{nx, ny, nz}
    , m_nghost{{0, 0}, {0, 0}, {0, 0}}
    , m_isUnstructured(false)
    , m_haveCoords(false)
    , m_computeNormals(false)
    , m_computeGhosts(computeGhost)
    {
        // allocate storage for normals
        addmappeddata((Scalar *)nullptr);
        addmappeddata((Scalar *)nullptr);
        addmappeddata((Scalar *)nullptr);

        addmappeddata(x);
        addmappeddata(y);
        addmappeddata(z);

        if (m_computeGhosts)
            addcelldata((Byte *)nullptr);
    }

    // for polygons
    HostData(Scalar isoValue, IsoDataFunctor isoFunc, const Index *el, const Index *cl, const Scalar *x,
             const Scalar *y, const Scalar *z)
    : m_isovalue(isoValue)
    , m_isoFunc(isoFunc)
    , m_el(el)
    , m_cl(cl)
    , m_nvert{0, 0, 0}
    , m_nghost{{0, 0}, {0, 0}, {0, 0}}
    , m_isUnstructured(false)
    , m_isPoly(true)
    , m_haveCoords(true)
    , m_computeNormals(false)
    {
        addmappeddata(x);
        addmappeddata(y);
        addmappeddata(z);
    }

    // for triangles/quads
    HostData(Scalar isoValue, IsoDataFunctor isoFunc, int vertPerCell, const Index *cl, const Scalar *x,
             const Scalar *y, const Scalar *z)
    : m_isovalue(isoValue)
    , m_isoFunc(isoFunc)
    , m_el(nullptr)
    , m_cl(cl)
    , m_tl(nullptr)
    , m_numVertPerCell(vertPerCell)
    , m_nvert{0, 0, 0}
    , m_nghost{{0, 0}, {0, 0}, {0, 0}}
    , m_isTri(vertPerCell == 3)
    , m_isQuad(vertPerCell == 4)
    , m_haveCoords(false)
    , m_computeNormals(false)
    {
        addmappeddata(x);
        addmappeddata(y);
        addmappeddata(z);
    }


    void setHaveCoords(bool val) { m_haveCoords = val; }

    void setGhostLayers(Index ghost[3][2])
    {
        for (int c = 0; c < 3; ++c) {
            for (int i = 0; i < 2; ++i) {
                m_nghost[c][i] = ghost[c][i];
            }
        }
    }

    void setComputeNormals(bool val) { m_computeNormals = val; }

    void addmappeddata(const Scalar *mapdata)
    {
        m_inVertPtr.push_back(mapdata);
        m_outVertData.emplace_back(vistle::ShmVector<Scalar>::create(0));
        m_outVertPtr.push_back(NULL);
        m_numInVertData = m_inVertPtr.size();
    }

    void addmappeddata(const Index *mapdata)
    {
        m_inVertPtrI.push_back(mapdata);
        m_outVertDataI.push_back(vistle::ShmVector<Index>::create(0));
        m_outVertPtrI.push_back(NULL);
        m_numInVertDataI = m_inVertPtrI.size();
    }

    void addmappeddata(const Byte *mapdata)
    {
        m_inVertPtrB.push_back(mapdata);
        m_outVertDataB.push_back(vistle::ShmVector<Byte>::create(0));
        m_outVertPtrB.push_back(NULL);
        m_numInVertDataB = m_inVertPtrB.size();
    }

    void addcelldata(const Scalar *mapdata)
    {
        m_inCellPtr.push_back(mapdata);
        m_outCellData.push_back(vistle::ShmVector<Scalar>::create(0));
        m_outCellPtr.push_back(NULL);
        m_numInCellData = m_inCellPtr.size();
    }

    void addcelldata(const Index *mapdata)
    {
        m_inCellPtrI.push_back(mapdata);
        m_outCellDataI.push_back(vistle::ShmVector<Index>::create(0));
        m_outCellPtrI.push_back(NULL);
        m_numInCellDataI = m_inCellPtrI.size();
    }

    void addcelldata(const Byte *mapdata)
    {
        m_inCellPtrB.push_back(mapdata);
        m_outCellDataB.push_back(vistle::ShmVector<Byte>::create(0));
        m_outCellPtrB.push_back(NULL);
        m_numInCellDataB = m_inCellPtrB.size();
    }
};

template<class Data>
struct ComputeOutput {
    ComputeOutput(Data &data): m_data(data)
    {
        for (int i = 0; i < m_data.m_numInVertData; i++) {
            m_data.m_outVertPtr[i] = m_data.m_outVertData[i]->data();
        }
        for (int i = 0; i < m_data.m_numInVertDataI; i++) {
            m_data.m_outVertPtrI[i] = m_data.m_outVertDataI[i]->data();
        }
        for (int i = 0; i < m_data.m_numInVertDataB; i++) {
            m_data.m_outVertPtrB[i] = m_data.m_outVertDataB[i]->data();
        }
        for (int i = 0; i < m_data.m_numInCellData; i++) {
            m_data.m_outCellPtr[i] = m_data.m_outCellData[i]->data();
        }
        for (int i = 0; i < m_data.m_numInCellDataI; i++) {
            m_data.m_outCellPtrI[i] = m_data.m_outCellDataI[i]->data();
        }
        for (int i = 0; i < m_data.m_numInCellDataB; i++) {
            m_data.m_outCellPtrB[i] = m_data.m_outCellDataB[i]->data();
        }
    }

    Data &m_data;

    void operator()(Index ValidCellIndex)
    {
        const Index CellNr = m_data.m_SelectedCellVector[ValidCellIndex];

        Byte ghost = cell::NORMAL;
        bool computeGhosts = m_data.m_computeGhosts && !m_data.m_isUnstructured;
        if (computeGhosts) {
            auto cc = vistle::StructuredGridBase::cellCoordinates(CellNr, m_data.m_nvert);
            for (int c = 0; c < 3; ++c) {
                if (cc[c] < m_data.m_nghost[c][0])
                    ghost = cell::GHOST;
                if (cc[c] + m_data.m_nghost[c][1] + 1 >= m_data.m_nvert[c])
                    ghost = cell::GHOST;
            }
        }

        for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex] / 3; idx++) {
            Index outcellindex = m_data.m_LocationList[ValidCellIndex] / 3 + idx;
            for (int j = 0; j < m_data.m_numInCellData; j++) {
                m_data.m_outCellPtr[j][outcellindex] = m_data.m_inCellPtr[j][CellNr];
            }
            for (int j = 0; j < m_data.m_numInCellDataI; j++) {
                m_data.m_outCellPtrI[j][outcellindex] = m_data.m_inCellPtrI[j][CellNr];
            }
            for (int j = 0; j < m_data.m_numInCellDataB; j++) {
                if (computeGhosts && j == 0) {
                    m_data.m_outCellPtrB[j][outcellindex] = ghost;
                } else {
                    m_data.m_outCellPtrB[j][outcellindex] = m_data.m_inCellPtrB[j][CellNr];
                }
            }
        }

        if (m_data.m_isUnstructured) {
            const Index Cellbegin = m_data.m_el[CellNr];
            const Index Cellend = m_data.m_el[CellNr + 1];
            const auto &cl = &m_data.m_cl[Cellbegin];

#define INTER(nc, triTable, edgeTable) \
    const unsigned int edge = triTable[m_data.m_caseNums[ValidCellIndex]][idx]; \
    const unsigned int v1 = edgeTable[0][edge]; \
    const unsigned int v2 = edgeTable[1][edge]; \
    const Scalar t = interpolation_weight<Scalar>(field[v1], field[v2], m_data.m_isovalue); \
    Index outvertexindex = m_data.m_LocationList[ValidCellIndex] + idx; \
    for (int j = nc; j < m_data.m_numInVertData; j++) { \
        m_data.m_outVertPtr[j][outvertexindex] = \
            lerp(m_data.m_inVertPtr[j][cl[v1]], m_data.m_inVertPtr[j][cl[v2]], t); \
    } \
    for (int j = 0; j < m_data.m_numInVertDataI; j++) { \
        m_data.m_outVertPtrI[j][outvertexindex] = \
            lerp(m_data.m_inVertPtrI[j][cl[v1]], m_data.m_inVertPtrI[j][cl[v2]], t); \
    } \
    for (int j = 0; j < m_data.m_numInVertDataB; j++) { \
        m_data.m_outVertPtrB[j][outvertexindex] = \
            lerp(m_data.m_inVertPtrB[j][cl[v1]], m_data.m_inVertPtrB[j][cl[v2]], t); \
    }

            switch (m_data.m_tl[CellNr]) {
            case UnstructuredGrid::HEXAHEDRON: {
                Scalar field[8];
                for (int idx = 0; idx < 8; idx++) {
                    field[idx] = m_data.m_isoFunc(cl[idx]);
                }

                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(0, hexaTriTable, hexaEdgeTable);
                }
                break;
            }

            case UnstructuredGrid::TETRAHEDRON: {
                Scalar field[4];
                for (int idx = 0; idx < 4; idx++) {
                    field[idx] = m_data.m_isoFunc(cl[idx]);
                }

                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(0, tetraTriTable, tetraEdgeTable);
                }
                break;
            }

            case UnstructuredGrid::PYRAMID: {
                Scalar field[5];
                for (int idx = 0; idx < 5; idx++) {
                    field[idx] = m_data.m_isoFunc(cl[idx]);
                }

                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(0, pyrTriTable, pyrEdgeTable);
                }
                break;
            }

            case UnstructuredGrid::PRISM: {
                Scalar field[6];
                for (int idx = 0; idx < 6; idx++) {
                    field[idx] = m_data.m_isoFunc(cl[idx]);
                }

                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(0, prismTriTable, prismEdgeTable);
                }
                break;
            }

            case UnstructuredGrid::POLYHEDRON: {
                /* find all iso-points on each edge of each face,
               build a triangle for each consecutive pair and a center point,
               orient outwards towards smaller values */

                const auto &cl = m_data.m_cl;

                const Index numVert = m_data.m_numVertices[ValidCellIndex];
                Index numAvg = 0;
                Scalar middleData[MaxNumData];
                Index middleDataI[MaxNumData];
                Byte middleDataB[MaxNumData];
                for (int i = 0; i < MaxNumData; i++) {
                    middleData[i] = 0;
                    middleDataI[i] = 0;
                    middleDataB[i] = 0;
                };
                Scalar cd1[MaxNumData], cd2[MaxNumData];
                Index cd1I[MaxNumData], cd2I[MaxNumData];
                Byte cd1B[MaxNumData], cd2B[MaxNumData];

                Index outIdx = m_data.m_LocationList[ValidCellIndex];
                Index facestart = InvalidIndex;
                Index term = 0;
                for (Index i = Cellbegin; i < Cellend; ++i) {
                    if (facestart == InvalidIndex) {
                        facestart = i;
                        term = cl[i];
                    } else if (term == cl[i]) {
                        const Index nvert = i - facestart;
                        bool flipped = false, haveIsect = false;
                        for (Index k = facestart; k < facestart + nvert; ++k) {
                            const Index c1 = cl[k];
                            const Index c2 = cl[k + 1];

                            for (int i = 0; i < m_data.m_numInVertData; i++) {
                                cd1[i] = m_data.m_inVertPtr[i][c1];
                                cd2[i] = m_data.m_inVertPtr[i][c2];
                            }
                            for (int i = 0; i < m_data.m_numInVertDataI; i++) {
                                cd1I[i] = m_data.m_inVertPtrI[i][c1];
                                cd2I[i] = m_data.m_inVertPtrI[i][c2];
                            }
                            for (int i = 0; i < m_data.m_numInVertDataB; i++) {
                                cd1B[i] = m_data.m_inVertPtrB[i][c1];
                                cd2B[i] = m_data.m_inVertPtrB[i][c2];
                            }

                            Scalar d1 = m_data.m_isoFunc(c1);
                            Scalar d2 = m_data.m_isoFunc(c2);

                            bool smallToBig = d1 <= m_data.m_isovalue && d2 > m_data.m_isovalue;
                            bool bigToSmall = d1 > m_data.m_isovalue && d2 <= m_data.m_isovalue;

                            if (smallToBig || bigToSmall) {
                                if (!haveIsect) {
                                    flipped = bigToSmall;
                                    haveIsect = true;
                                }
                                Index out = outIdx;
                                if (flipped) {
                                    if (bigToSmall)
                                        out += 1;
                                    else
                                        out -= 1;
                                }
                                Scalar t = interpolation_weight<Scalar>(d1, d2, m_data.m_isovalue);
                                for (int i = 0; i < m_data.m_numInVertData; i++) {
                                    Scalar v = lerp(cd1[i], cd2[i], t);
                                    middleData[i] += v;
                                    m_data.m_outVertPtr[i][out] = v;
                                }
                                for (int i = 0; i < m_data.m_numInVertDataI; i++) {
                                    Index vI = lerp(cd1I[i], cd2I[i], t);
                                    middleDataI[i] += vI;
                                    m_data.m_outVertPtrI[i][out] = vI;
                                }
                                for (int i = 0; i < m_data.m_numInVertDataB; i++) {
                                    Byte vB = lerp(cd1B[i], cd2B[i], t);
                                    middleDataB[i] += vB;
                                    m_data.m_outVertPtrB[i][out] = vB;
                                }

                                ++outIdx;
                                if (bigToSmall ^ flipped)
                                    ++outIdx;
                                ++numAvg;
                            }
                        }
                        facestart = InvalidIndex;
                    }
                }
                if (numAvg > 0) {
                    for (int i = 0; i < m_data.m_numInVertData; i++) {
                        middleData[i] /= numAvg;
                    }
                    for (int i = 0; i < m_data.m_numInVertDataI; i++) {
                        middleDataI[i] /= numAvg;
                    }
                    for (int i = 0; i < m_data.m_numInVertDataB; i++) {
                        middleDataB[i] /= numAvg;
                    }
                }
                for (Index i = 2; i < numVert; i += 3) {
                    const Index idx = m_data.m_LocationList[ValidCellIndex] + i;
                    for (int i = 0; i < m_data.m_numInVertData; i++) {
                        m_data.m_outVertPtr[i][idx] = middleData[i];
                    }
                    for (int i = 0; i < m_data.m_numInVertDataI; i++) {
                        m_data.m_outVertPtrI[i][idx] = middleDataI[i];
                    }
                    for (int i = 0; i < m_data.m_numInVertDataB; i++) {
                        m_data.m_outVertPtrB[i][idx] = middleDataB[i];
                    }
                }
                break;
            }
            }
        } else if (m_data.m_isTri) {
#define INTERFLAT(nc, triTable, edgeTable) \
    const unsigned int edge = triTable[m_data.m_caseNums[ValidCellIndex]][idx]; \
    const unsigned int v1 = edgeTable[0][edge]; \
    const unsigned int v2 = edgeTable[1][edge]; \
    const Scalar t = interpolation_weight<Scalar>(field[v1], field[v2], m_data.m_isovalue); \
    Index outvertexindex = m_data.m_LocationList[ValidCellIndex] + idx; \
    for (int j = nc; j < m_data.m_numInVertData; j++) { \
        m_data.m_outVertPtr[j][outvertexindex] = \
            lerp(m_data.m_inVertPtr[j][base + v1], m_data.m_inVertPtr[j][base + v2], t); \
    } \
    for (int j = 0; j < m_data.m_numInVertDataI; j++) { \
        m_data.m_outVertPtrI[j][outvertexindex] = \
            lerp(m_data.m_inVertPtrI[j][base + v1], m_data.m_inVertPtrI[j][base + v2], t); \
    } \
    for (int j = 0; j < m_data.m_numInVertDataB; j++) { \
        m_data.m_outVertPtrB[j][outvertexindex] = \
            lerp(m_data.m_inVertPtrB[j][base + v1], m_data.m_inVertPtrB[j][base + v2], t); \
    }
            const Index Cellbegin = CellNr * 3;
            Scalar field[3];
            if (m_data.m_cl) {
                const auto &cl = &m_data.m_cl[Cellbegin];
                for (int idx = 0; idx < 3; idx++) {
                    field[idx] = m_data.m_isoFunc(cl[idx]);
                }
                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(0, triLineTable, triEdgeTable);
                }
            } else {
                const Index base = Cellbegin;
                for (int idx = 0; idx < 3; idx++) {
                    field[idx] = m_data.m_isoFunc(base + idx);
                }
                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTERFLAT(0, triLineTable, triEdgeTable);
                }
            }
        } else if (m_data.m_isQuad) {
            const Index Cellbegin = CellNr * 4;
            Scalar field[4];
            if (m_data.m_cl) {
                const auto &cl = &m_data.m_cl[Cellbegin];
                for (int idx = 0; idx < 4; idx++) {
                    field[idx] = m_data.m_isoFunc(cl[idx]);
                }
                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(0, quadLineTable, quadEdgeTable);
                }
            } else {
                const Index base = Cellbegin;
                for (int idx = 0; idx < 4; idx++) {
                    field[idx] = m_data.m_isoFunc(base + idx);
                }
                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTERFLAT(0, quadLineTable, quadEdgeTable);
                }
            }
        } else if (m_data.m_isPoly) {
            const Index CellBegin = m_data.m_el[CellNr];
            const Index CellEnd = m_data.m_el[CellNr + 1];
            const auto &cl = &m_data.m_cl[0];

            Index outIdx = m_data.m_LocationList[ValidCellIndex];
            Index c1 = cl[CellEnd - 1];
            for (Index k = CellBegin; k < CellEnd; ++k) {
                const Index c2 = cl[k];

                Scalar d1 = m_data.m_isoFunc(c1);
                Scalar d2 = m_data.m_isoFunc(c2);

                bool smallToBig = d1 <= m_data.m_isovalue && d2 > m_data.m_isovalue;
                bool bigToSmall = d1 > m_data.m_isovalue && d2 <= m_data.m_isovalue;
                if (smallToBig || bigToSmall) {
                    Scalar t = interpolation_weight<Scalar>(d1, d2, m_data.m_isovalue);
                    for (int i = 0; i < m_data.m_numInVertData; i++) {
                        Scalar v = lerp(m_data.m_inVertPtr[i][c1], m_data.m_inVertPtr[i][c2], t);
                        m_data.m_outVertPtr[i][outIdx] = v;
                    }
                    for (int i = 0; i < m_data.m_numInVertDataI; i++) {
                        Index vI = lerp(m_data.m_inVertPtrI[i][c1], m_data.m_inVertPtrI[i][c2], t);
                        m_data.m_outVertPtrI[i][outIdx] = vI;
                    }
                    for (int i = 0; i < m_data.m_numInVertDataB; i++) {
                        Byte vB = lerp(m_data.m_inVertPtrB[i][c1], m_data.m_inVertPtrB[i][c2], t);
                        m_data.m_outVertPtrB[i][outIdx] = vB;
                    }

                    ++outIdx;
                }
                c1 = c2;
            }
        } else {
            auto cl = vistle::StructuredGridBase::cellVertices(CellNr, m_data.m_nvert);
            assert(cl.size() <= 8);
            Scalar field[8];
            for (unsigned idx = 0; idx < cl.size(); idx++) {
                field[idx] = m_data.m_isoFunc(cl[idx]);
            }


            Scalar grad[8][3];
            if (m_data.m_computeNormals) {
                const auto &H = StructuredGridBase::HexahedronIndices;
                auto n = vistle::StructuredGridBase::cellCoordinates(CellNr, m_data.m_nvert);
                for (int idx = 0; idx < 8; idx++) {
                    Index x[3], xl[3], xu[3];
                    for (int c = 0; c < 3; ++c) {
                        x[c] = n[c] + H[c][idx];
                    }
                    for (int c = 0; c < 3; ++c) {
                        xl[c] = x[c] > 0 ? x[c] - 1 : x[c];
                        xu[c] = x[c] < m_data.m_nvert[c] - 1 ? x[c] + 1 : x[c];
                    }
                    for (int c = 0; c < 3; ++c) {
                        Index xx = x[c];
                        x[c] = xl[c];
                        Index l = StructuredGridBase::vertexIndex(x, m_data.m_nvert);
                        x[c] = xu[c];
                        Index u = StructuredGridBase::vertexIndex(x, m_data.m_nvert);
                        x[c] = xx;
                        grad[idx][c] = m_data.m_isoFunc(u) - m_data.m_isoFunc(l);
                        Scalar diff = 0;
                        if (m_data.m_haveCoords) {
                            diff = (m_data.m_inVertPtr[3 + c][u] - m_data.m_inVertPtr[3 + c][l]);
                        } else {
                            diff = (m_data.m_inVertPtr[3 + c][xu[c]] - m_data.m_inVertPtr[3 + c][xl[c]]);
                        }
                        if (fabs(diff) > EPSILON)
                            grad[idx][c] /= diff;
                        else
                            grad[idx][c] = 0;
                    }
                }
            }

            if (m_data.m_haveCoords) {
                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(3, hexaTriTable, hexaEdgeTable);
                    if (m_data.m_computeNormals) {
                        for (int j = 0; j < 3; j++) {
                            m_data.m_outVertPtr[j][outvertexindex] = lerp(grad[v1][j], grad[v2][j], t);
                        }
                    }
                }
            } else {
                for (Index idx = 0; idx < m_data.m_numVertices[ValidCellIndex]; idx++) {
                    INTER(6, hexaTriTable, hexaEdgeTable);

                    if (m_data.m_computeNormals) {
                        for (int j = 0; j < 3; j++) {
                            m_data.m_outVertPtr[j][outvertexindex] = lerp(grad[v1][j], grad[v2][j], t);
                        }
                    }

                    auto vc1 = StructuredGridBase::vertexCoordinates(cl[v1], m_data.m_nvert);
                    auto vc2 = StructuredGridBase::vertexCoordinates(cl[v2], m_data.m_nvert);
                    for (int j = 0; j < 3; j++) {
                        m_data.m_outVertPtr[3 + j][outvertexindex] =
                            lerp(m_data.m_inVertPtr[3 + j][vc1[j]], m_data.m_inVertPtr[3 + j][vc2[j]], t);
                    }
                }
            }
        }
    }
};

template<class Data>
struct SelectCells {
    Data &m_data;
    SelectCells(Data &data): m_data(data) {}

    // for unstructured grids
    int operator()(const thrust::tuple<Index, Index> iCell) const
    {
        int havelower = 0;
        int havehigher = 0;
        Index Cell = iCell.get<0>();
        Index nextCell = iCell.get<1>();
        // also for POLYHEDRON
        for (Index i = Cell; i < nextCell; i++) {
            Scalar val = m_data.m_isoFunc(m_data.m_cl[i]);
            if (val > m_data.m_isovalue) {
                havelower = 1;
                if (havehigher)
                    return 1;
            } else {
                havehigher = 1;
                if (havelower)
                    return 1;
            }
        }
        return 0;
    }

    // for all types of structured grids
    int operator()(const Index Cell) const
    {
        auto verts = vistle::StructuredGridBase::cellVertices(Cell, m_data.m_nvert);
        int havelower = 0;
        int havehigher = 0;
        for (int i = 0; i < 8; ++i) {
            Scalar val = m_data.m_isoFunc(verts[i]);
            if (val > m_data.m_isovalue) {
                havelower = 1;
                if (havehigher)
                    return 1;
            } else {
                havehigher = 1;
                if (havelower)
                    return 1;
            }
        }
        return 0;
    }
};

template<class Data>
struct SelectCells2D {
    Data &m_data;
    SelectCells2D(Data &data): m_data(data) {}

    // for polygons
    int operator()(const thrust::tuple<Index, Index> iCell) const
    {
        int havelower = 0;
        int havehigher = 0;
        Index Cell = iCell.get<0>();
        Index nextCell = iCell.get<1>();
        for (Index i = Cell; i < nextCell; i++) {
            Scalar val = m_data.m_isoFunc(m_data.m_cl[i]);
            if (val > m_data.m_isovalue) {
                havelower = 1;
                if (havehigher)
                    return 1;
            } else {
                havehigher = 1;
                if (havelower)
                    return 1;
            }
        }
        return 0;
    }

    // for triangles and quads
    int operator()(const Index Cell) const
    {
        int havelower = 0;
        int havehigher = 0;
        Index begin = Cell * m_data.m_numVertPerCell, end = begin + m_data.m_numVertPerCell;
        if (m_data.m_cl) {
            for (Index i = begin; i < end; ++i) {
                Scalar val = m_data.m_isoFunc(m_data.m_cl[i]);
                if (val > m_data.m_isovalue) {
                    havelower = 1;
                    if (havehigher)
                        return 1;
                } else {
                    havehigher = 1;
                    if (havelower)
                        return 1;
                }
            }
        } else {
            for (Index i = begin; i < end; ++i) {
                Scalar val = m_data.m_isoFunc(i);
                if (val > m_data.m_isovalue) {
                    havelower = 1;
                    if (havehigher)
                        return 1;
                } else {
                    havehigher = 1;
                    if (havelower)
                        return 1;
                }
            }
        }
        return 0;
    }
};


template<class Data>
struct ComputeOutputSizes {
    ComputeOutputSizes(Data &data): m_data(data) {}

    Data &m_data;

    thrust::tuple<Index, Index> operator()(Index CellNr)
    {
        int tableIndex = 0;
        unsigned numVerts = 0;
        if (m_data.m_isUnstructured) {
            const auto &cl = m_data.m_cl;

            Index begin = m_data.m_el[CellNr], end = m_data.m_el[CellNr + 1];
            Index nvert = end - begin;
            Byte CellType = m_data.m_tl[CellNr];
            if (CellType != UnstructuredGrid::POLYHEDRON) {
                for (Index idx = 0; idx < nvert; idx++) {
                    tableIndex += (((int)(m_data.m_isoFunc(m_data.m_cl[begin + idx]) > m_data.m_isovalue)) << idx);
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
                Index vertcounter = 0;
                Index facestart = InvalidIndex;
                Index term = 0;
                for (Index i = begin; i < end; ++i) {
                    if (facestart == InvalidIndex) {
                        facestart = i;
                        term = cl[i];
                    } else if (term == cl[i]) {
                        const Index N = i - facestart;
                        Index prev = cl[facestart + N - 1];
                        for (Index k = facestart; k < facestart + N; ++k) {
                            Index v = cl[k];

                            if (m_data.m_isoFunc(prev) <= m_data.m_isovalue &&
                                m_data.m_isoFunc(v) > m_data.m_isovalue) {
                                ++vertcounter;
                            } else if (m_data.m_isoFunc(prev) > m_data.m_isovalue &&
                                       m_data.m_isoFunc(v) <= m_data.m_isovalue) {
                                ++vertcounter;
                            }

                            prev = v;
                        }
                        facestart = InvalidIndex;
                    }
                }
                numVerts = vertcounter + vertcounter / 2;
                break;
            }
            }
        } else if (m_data.m_isTri || m_data.m_isQuad) {
            const auto &cl = m_data.m_cl;
            const Index begin = CellNr * m_data.m_numVertPerCell, end = begin + m_data.m_numVertPerCell;
            if (cl) {
                int idx = 0;
                for (Index i = begin; i < end; ++i) {
                    tableIndex += (((int)(m_data.m_isoFunc(cl[i]) > m_data.m_isovalue)) << idx);
                    ++idx;
                }
            } else {
                int idx = 0;
                for (Index i = begin; i < end; ++i) {
                    tableIndex += (((int)(m_data.m_isoFunc(i) > m_data.m_isovalue)) << idx);
                    ++idx;
                }
            }
            numVerts = m_data.m_isQuad ? quadNumVertsTable[tableIndex] : triNumVertsTable[tableIndex];
        } else if (m_data.m_isPoly) {
            const auto &cl = m_data.m_cl;
            Index begin = m_data.m_el[CellNr], end = m_data.m_el[CellNr + 1];
            Index vertcounter = 0;
            Index prev = cl[end - 1];
            for (Index i = begin; i < end; ++i) {
                const Index v = cl[i];
                if (m_data.m_isoFunc(prev) <= m_data.m_isovalue && m_data.m_isoFunc(v) > m_data.m_isovalue) {
                    ++vertcounter;
                } else if (m_data.m_isoFunc(prev) > m_data.m_isovalue && m_data.m_isoFunc(v) <= m_data.m_isovalue) {
                    ++vertcounter;
                }
                prev = v;
            }
            assert(vertcounter % 2 == 0);
            numVerts = vertcounter;
        } else {
            auto verts = vistle::StructuredGridBase::cellVertices(CellNr, m_data.m_nvert);
            assert(verts.size() <= 8);
            for (unsigned idx = 0; idx < verts.size(); ++idx) {
                tableIndex += (((int)(m_data.m_isoFunc(verts[idx]) > m_data.m_isovalue)) << idx);
            }
            numVerts = hexaNumVertsTable[tableIndex];
        }
        return thrust::make_tuple<Index, Index>(tableIndex, numVerts);
    }
};

Leveller::Leveller(const IsoController &isocontrol, Object::const_ptr grid, const Scalar isovalue)
: m_isocontrol(isocontrol)
, m_grid(grid)
, m_uni(UniformGrid::as(grid))
, m_lg(LayerGrid::as(grid))
, m_rect(RectilinearGrid::as(grid))
, m_str(StructuredGrid::as(grid))
, m_unstr(UnstructuredGrid::as(grid))
, m_strbase(StructuredGridBase::as(grid))
, m_poly(Polygons::as(grid))
, m_quad(Quads::as(grid))
, m_tri(Triangles::as(grid))
, m_coord(Coords::as(grid))
, m_isoValue(isovalue)
, gmin(std::numeric_limits<Scalar>::max())
, gmax(-std::numeric_limits<Scalar>::max())
, m_objectTransform(grid->getTransform())
{
    if (m_strbase || m_unstr) {
        m_triangles = Triangles::ptr(new Triangles(Object::Initialized));
        m_triangles->setMeta(grid->meta());
    } else if (m_poly || m_tri || m_quad) {
        m_lines = Lines::ptr(new Lines(Object::Initialized));
        m_lines->setMeta(grid->meta());
    }
}

template<class Data, class pol>
Index Leveller::calculateSurface(Data &data)
{
    Index nelem = 0;
    if (m_strbase) {
        nelem = m_strbase->getNumElements();
    } else if (m_unstr) {
        nelem = m_unstr->getNumElements();
    } else if (m_tri) {
        nelem = m_tri->getNumElements();
    } else if (m_quad) {
        nelem = m_quad->getNumElements();
    } else if (m_poly) {
        nelem = m_poly->getNumElements();
    }
    thrust::counting_iterator<Index> first(0), last = first + nelem;
    ;
    size_t numSelectedCells = 0;
    if (data.m_SelectedCellVectorValid) {
        numSelectedCells = data.m_SelectedCellVector.size();
    } else {
        data.m_SelectedCellVector.resize(nelem);


        typename Data::VectorIndexIterator end;
        if (m_strbase) {
            end = thrust::copy_if(pol(), first, last, thrust::counting_iterator<Index>(0),
                                  data.m_SelectedCellVector.begin(), SelectCells<Data>(data));
        } else if (m_unstr) {
            typedef thrust::tuple<typename Data::IndexIterator, typename Data::IndexIterator> Iteratortuple;
            typedef thrust::zip_iterator<Iteratortuple> ZipIterator;
            ZipIterator ElTupleVec(thrust::make_tuple(&data.m_el[0], &data.m_el[1]));
            end = thrust::copy_if(pol(), first, last, ElTupleVec, data.m_SelectedCellVector.begin(),
                                  SelectCells<Data>(data));
        } else if (m_poly) {
            typedef thrust::tuple<typename Data::IndexIterator, typename Data::IndexIterator> Iteratortuple;
            typedef thrust::zip_iterator<Iteratortuple> ZipIterator;
            ZipIterator ElTupleVec(thrust::make_tuple(&data.m_el[0], &data.m_el[1]));
            end = thrust::copy_if(pol(), first, last, ElTupleVec, data.m_SelectedCellVector.begin(),
                                  SelectCells2D<Data>(data));
        } else if (m_tri || m_quad) {
            end = thrust::copy_if(pol(), first, last, thrust::counting_iterator<Index>(0),
                                  data.m_SelectedCellVector.begin(), SelectCells2D<Data>(data));
        }

        numSelectedCells = end - data.m_SelectedCellVector.begin();
        data.m_SelectedCellVector.resize(numSelectedCells);
        data.m_SelectedCellVectorValid = true;
    }
    data.m_caseNums.resize(numSelectedCells);
    data.m_numVertices.resize(numSelectedCells);
    data.m_LocationList.resize(numSelectedCells);
    thrust::transform(
        pol(), data.m_SelectedCellVector.begin(), data.m_SelectedCellVector.end(),
        thrust::make_zip_iterator(thrust::make_tuple(data.m_caseNums.begin(), data.m_numVertices.begin())),
        ComputeOutputSizes<Data>(data));
    thrust::exclusive_scan(pol(), data.m_numVertices.begin(), data.m_numVertices.end(), data.m_LocationList.begin());
    Index totalNumVertices = 0;
    if (!data.m_numVertices.empty())
        totalNumVertices += data.m_numVertices.back();
    if (!data.m_LocationList.empty())
        totalNumVertices += data.m_LocationList.back();
    for (int i = (m_computeNormals || !m_strbase ? 0 : 3); i < data.m_numInVertData; i++) {
        data.m_outVertData[i]->resize(totalNumVertices);
    }
    for (int i = 0; i < data.m_numInVertDataI; i++) {
        data.m_outVertDataI[i]->resize(totalNumVertices);
    }
    for (int i = 0; i < data.m_numInVertDataB; i++) {
        data.m_outVertDataB[i]->resize(totalNumVertices);
    }
    for (int i = 0; i < data.m_numInCellData; ++i) {
        data.m_outCellData[i]->resize(totalNumVertices / 3);
    }
    for (int i = 0; i < data.m_numInCellDataI; ++i) {
        data.m_outCellDataI[i]->resize(totalNumVertices / 3);
    }
    for (int i = 0; i < data.m_numInCellDataB; ++i) {
        data.m_outCellDataB[i]->resize(totalNumVertices / 3);
    }
    thrust::counting_iterator<Index> start(0), finish(numSelectedCells);
    thrust::for_each(pol(), start, finish, ComputeOutput<Data>(data));

    return totalNumVertices;
}

bool Leveller::process()
{
#ifndef CUTTINGSURFACE
    Vec<Scalar>::const_ptr dataobj = Vec<Scalar>::as(m_data);
    if (!dataobj)
        return false;
    auto bounds = dataobj->getMinMax();
    if (bounds.first[0] <= bounds.second[0]) {
        if (m_isoValue < bounds.first[0] || m_isoValue > bounds.second[0])
            return true;
    }
#else
#endif

    Index dims[3] = {0, 0, 0};
    if (m_strbase) {
        dims[0] = m_strbase->getNumDivisions(0);
        dims[1] = m_strbase->getNumDivisions(1);
        dims[2] = m_strbase->getNumDivisions(2);
    }
    std::vector<Scalar> unicoords[3];
    const Scalar *coords[3]{nullptr, nullptr, nullptr};
    if (m_uni) {
        for (int i = 0; i < 3; ++i) {
            unicoords[i].resize(dims[i]);
            coords[i] = unicoords[i].data();
            Scalar dist = 0;
            if (dims[i] > 1)
                dist = (m_uni->max()[i] - m_uni->min()[i]) / (dims[i] - 1);
            Scalar val = m_uni->min()[i];
            for (Index j = 0; j < dims[i]; ++j) {
                unicoords[i][j] = val;
                val += dist;
            }
        }
    } else if (m_lg) {
        for (int i = 0; i < 2; ++i) {
            unicoords[i].resize(dims[i]);
            coords[i] = unicoords[i].data();
            Scalar dist = 0;
            if (dims[i] > 1)
                dist = (m_lg->max()[i] - m_lg->min()[i]) / (dims[i] - 1);
            Scalar val = m_lg->min()[i];
            for (Index j = 0; j < dims[i]; ++j) {
                unicoords[i][j] = val;
                val += dist;
            }
        }
        coords[2] = &m_lg->z()[0];
    } else if (m_rect) {
        for (int i = 0; i < 3; ++i)
            coords[i] = &m_rect->coords(i)[0];
    } else if (m_str) {
        for (int i = 0; i < 3; ++i)
            coords[i] = &m_str->x(i)[0];
    }
#ifdef CUTTINGSURFACE
    IsoDataFunctor isofunc =
        m_coord ? m_isocontrol.newFunc(m_grid->getTransform(), &m_coord->x()[0], &m_coord->y()[0], &m_coord->z()[0])
                : m_isocontrol.newFunc(m_grid->getTransform(), dims, coords[0], coords[1], coords[2]);
#else
    IsoDataFunctor isofunc = m_isocontrol.newFunc(m_grid->getTransform(), &dataobj->x()[0]);
#endif

    std::unique_ptr<HostData> HD_ptr;
    if (m_unstr) {
        const Byte *ghost = nullptr;
        if (m_unstr->ghost().size() > 0) {
            ghost = m_unstr->ghost().data();
        }
        HD_ptr = std::make_unique<HostData>(m_isoValue, isofunc, m_unstr->el(), m_unstr->tl(), ghost, m_unstr->cl(),
                                            m_unstr->x(), m_unstr->y(), m_unstr->z());

    } else if (m_strbase) {
        bool haveGhost = false;
        Index ghost[3][2];
        for (int c = 0; c < 3; ++c) {
            ghost[c][0] = m_strbase->getNumGhostLayers(c, StructuredGridBase::Bottom);
            if (ghost[c][0] > 0)
                haveGhost = true;
            ghost[c][1] = m_strbase->getNumGhostLayers(c, StructuredGridBase::Top);
            if (ghost[c][1] > 0)
                haveGhost = true;
        }
        HD_ptr = std::make_unique<HostData>(m_isoValue, isofunc, dims[0], dims[1], dims[2], coords[0], coords[1],
                                            coords[2], haveGhost);
        HD_ptr->setGhostLayers(ghost);

    } else if (m_poly) {
        HD_ptr = std::make_unique<HostData>(m_isoValue, isofunc, m_poly->el(), m_poly->cl(), m_poly->x(), m_poly->y(),
                                            m_poly->z());

    } else if (m_quad) {
        HD_ptr =
            std::make_unique<HostData>(m_isoValue, isofunc, 4, m_quad->cl(), m_quad->x(), m_quad->y(), m_quad->z());

    } else if (m_tri) {
        HD_ptr = std::make_unique<HostData>(m_isoValue, isofunc, 3, m_tri->cl(), m_tri->x(), m_tri->y(), m_tri->z());
    } else {
        return false;
    }
    HostData &HD = *HD_ptr;
    HD.setHaveCoords(m_coord ? true : false);
    HD.setComputeNormals(m_computeNormals);
#ifdef CUTTINGSURFACE
    if (m_candidateCells) {
        HD.m_SelectedCellVector = *m_candidateCells;
        HD.m_SelectedCellVectorValid = true;
    }
#endif

    for (size_t i = 0; i < m_vertexdata.size(); ++i) {
        if (Vec<Scalar, 1>::const_ptr Scal = Vec<Scalar, 1>::as(m_vertexdata[i])) {
            HD.addmappeddata(Scal->x());
        }
        if (Vec<Scalar, 3>::const_ptr Vect = Vec<Scalar, 3>::as(m_vertexdata[i])) {
            HD.addmappeddata(Vect->x());
            HD.addmappeddata(Vect->y());
            HD.addmappeddata(Vect->z());
        }
        if (Vec<Index, 1>::const_ptr Idx = Vec<Index, 1>::as(m_vertexdata[i])) {
            HD.addmappeddata(Idx->x());
        }
        if (auto byte = Vec<Byte>::as(m_vertexdata[i])) {
            HD.addmappeddata(byte->x());
        }
    }

    for (size_t i = 0; i < m_celldata.size(); ++i) {
        if (Vec<Scalar, 1>::const_ptr Scal = Vec<Scalar, 1>::as(m_celldata[i])) {
            HD.addcelldata(Scal->x());
        }
        if (Vec<Scalar, 3>::const_ptr Vect = Vec<Scalar, 3>::as(m_celldata[i])) {
            HD.addcelldata(Vect->x());
            HD.addcelldata(Vect->y());
            HD.addcelldata(Vect->z());
        }
        if (Vec<Index, 1>::const_ptr Idx = Vec<Index, 1>::as(m_celldata[i])) {
            HD.addcelldata(Idx->x());
        }
        if (auto byte = Vec<Byte>::as(m_celldata[i])) {
            HD.addcelldata(byte->x());
        }
    }

    Index totalNumVertices = calculateSurface<HostData, thrust::detail::host_t>(HD);

    {
        size_t idx = 0;
        if (m_strbase) {
            if (m_computeNormals) {
                m_normals.reset(new Normals(Object::Initialized));
                m_normals->d()->x[0] = HD.m_outVertData[idx++];
                m_normals->d()->x[1] = HD.m_outVertData[idx++];
                m_normals->d()->x[2] = HD.m_outVertData[idx++];
                m_normals->setMapping(DataBase::Vertex);
            } else {
                idx = 3;
            }
        }

        if (m_triangles) {
            m_triangles->d()->x[0] = HD.m_outVertData[idx++];
            m_triangles->d()->x[1] = HD.m_outVertData[idx++];
            m_triangles->d()->x[2] = HD.m_outVertData[idx++];
        } else if (m_lines) {
            m_lines->d()->x[0] = HD.m_outVertData[idx++];
            m_lines->d()->x[1] = HD.m_outVertData[idx++];
            m_lines->d()->x[2] = HD.m_outVertData[idx++];
            std::cerr << "lines with " << totalNumVertices << " vertices" << std::endl;
            auto &cl = m_lines->cl();
            auto &el = m_lines->el();
            for (Index i = 0; i < totalNumVertices; ++i) {
                cl.emplace_back(i);
                if (cl.size() % 2 == 0)
                    el.emplace_back(cl.size());
            }
        }

        size_t idxI = 0, idxB = 0;
        for (size_t i = 0; i < m_vertexdata.size(); ++i) {
            if (Vec<Scalar>::as(m_vertexdata[i])) {
                Vec<Scalar, 1>::ptr out = Vec<Scalar, 1>::ptr(new Vec<Scalar, 1>(Object::Initialized));
                out->d()->x[0] = HD.m_outVertData[idx++];
                out->setMeta(m_vertexdata[i]->meta());
                out->setMapping(DataBase::Vertex);
                m_outvertData.push_back(out);
            }
            if (Vec<Scalar, 3>::as(m_vertexdata[i])) {
                Vec<Scalar, 3>::ptr out = Vec<Scalar, 3>::ptr(new Vec<Scalar, 3>(Object::Initialized));
                out->d()->x[0] = HD.m_outVertData[idx++];
                out->d()->x[1] = HD.m_outVertData[idx++];
                out->d()->x[2] = HD.m_outVertData[idx++];
                out->setMeta(m_vertexdata[i]->meta());
                out->setMapping(DataBase::Vertex);
                m_outvertData.push_back(out);
            }
            if (Vec<Index>::as(m_vertexdata[i])) {
                Vec<Index>::ptr out = Vec<Index>::ptr(new Vec<Index>(Object::Initialized));
                out->d()->x[0] = HD.m_outVertDataI[idxI++];
                out->setMeta(m_vertexdata[i]->meta());
                out->setMapping(DataBase::Vertex);
                m_outvertData.push_back(out);
            }
            if (Vec<Byte>::as(m_vertexdata[i])) {
                Vec<Byte>::ptr out = Vec<Byte>::ptr(new Vec<Byte>(Object::Initialized));
                out->d()->x[0] = HD.m_outVertDataB[idxB++];
                out->setMeta(m_vertexdata[i]->meta());
                out->setMapping(DataBase::Vertex);
                m_outvertData.push_back(out);
            }
        }
    }
    {
        size_t idx = 0;
        size_t idxI = 0;
        size_t idxB = 0;
        if (HD.m_computeGhosts) {
            assert(m_triangles);
            assert(m_unstr || m_strbase);
            m_triangles->d()->ghost = HD.m_outCellDataB[idxB++];
        }
        for (size_t i = 0; i < m_celldata.size(); ++i) {
            if (Vec<Scalar>::as(m_celldata[i])) {
                Vec<Scalar, 1>::ptr out = Vec<Scalar, 1>::ptr(new Vec<Scalar, 1>(Object::Initialized));
                out->d()->x[0] = HD.m_outCellData[idx++];
                out->setMeta(m_celldata[i]->meta());
                out->setMapping(DataBase::Element);
                m_outcellData.push_back(out);
            }
            if (Vec<Scalar, 3>::as(m_celldata[i])) {
                Vec<Scalar, 3>::ptr out = Vec<Scalar, 3>::ptr(new Vec<Scalar, 3>(Object::Initialized));
                out->d()->x[0] = HD.m_outCellData[idx++];
                out->d()->x[1] = HD.m_outCellData[idx++];
                out->d()->x[2] = HD.m_outCellData[idx++];
                out->setMeta(m_celldata[i]->meta());
                out->setMapping(DataBase::Element);
                m_outcellData.push_back(out);
            }
            if (Vec<Index>::as(m_celldata[i])) {
                Vec<Index>::ptr out = Vec<Index>::ptr(new Vec<Index>(Object::Initialized));
                out->d()->x[0] = HD.m_outCellDataI[idxI++];
                out->setMeta(m_celldata[i]->meta());
                out->setMapping(DataBase::Element);
                m_outcellData.push_back(out);
            }
            if (Vec<Byte>::as(m_celldata[i])) {
                Vec<Byte>::ptr out = Vec<Byte>::ptr(new Vec<Byte>(Object::Initialized));
                out->d()->x[0] = HD.m_outCellDataB[idxB++];
                out->setMeta(m_celldata[i]->meta());
                out->setMapping(DataBase::Element);
                m_outcellData.push_back(out);
            }
        }
    }

#ifdef CUTTINGSURFACE
    if (!m_candidateCells && HD.m_SelectedCellVectorValid) {
        m_candidateCells = new std::vector<vistle::Index>(HD.m_SelectedCellVector);
    }
#endif

    return true;
}

#ifndef CUTTINGSURFACE
void Leveller::setIsoData(Vec<Scalar>::const_ptr obj)
{
    m_data = obj;
}
#endif

void Leveller::setComputeNormals(bool value)
{
    m_computeNormals = value;
}

void Leveller::addMappedData(DataBase::const_ptr mapobj)
{
    if (mapobj->mapping() == DataBase::Element)
        m_celldata.push_back(mapobj);
    else
        m_vertexdata.push_back(mapobj);
}

Coords::ptr Leveller::result()
{
    if (m_triangles)
        return m_triangles;
    return m_lines;
}

Normals::ptr Leveller::normresult()
{
    return m_normals;
}

DataBase::ptr Leveller::mapresult() const
{
    if (m_outvertData.size())
        return m_outvertData[0];
    else if (m_outcellData.size())
        return m_outcellData[0];
    else
        return DataBase::ptr();
}

DataBase::ptr Leveller::cellresult() const
{
    if (m_outcellData.size())
        return m_outcellData[0];
    else
        return DataBase::ptr();
}

std::pair<Scalar, Scalar> Leveller::range()
{
    return std::make_pair(gmin, gmax);
}

#ifdef CUTTINGSURFACE
const std::vector<Index> *Leveller::candidateCells()
{
    return m_candidateCells;
}

void Leveller::setCandidateCells(const std::vector<Index> *candidateCells)
{
    m_candidateCells = candidateCells;
}
#endif
