#ifndef UNSTRUCTUREDGRID_H
#define UNSTRUCTUREDGRID_H


#include "shm.h"
#include "indexed.h"
#include "grid.h"
#include "celltypes.h"
#include <vistle/util/enum.h>

namespace vistle {

class V_COREEXPORT UnstructuredGrid: public Indexed, virtual public GridInterface {
    V_OBJECT(UnstructuredGrid);

public:
    typedef Indexed Base;
    enum Type {
        CONVEX_BIT = cell::CONVEX_BIT,
        TYPE_MASK = cell::TYPE_MASK,

        NONE = cell::NONE, // 0
        POINT = cell::POINT, // 1
        BAR = cell::BAR, // 3
        TRIANGLE = cell::TRIANGLE, // 5
        POLYGON = cell::POLYGON, // 7
        QUAD = cell::QUAD, // 9
        TETRAHEDRON = cell::TETRAHEDRON, // 10
        POLYHEDRON = cell::POLYHEDRON, // 11
        HEXAHEDRON = cell::HEXAHEDRON, // 12
        PRISM = cell::PRISM, // 13
        PYRAMID = cell::PYRAMID, // 14
        NUM_TYPES = cell::NUM_TYPES, // 15
    };

    static constexpr Index MaxNumVertices = 4;
    static constexpr Index MaxNumFaces = 6;
    static constexpr int Dimensionality[NUM_TYPES] = {
        -1, 0, -1, 1, -1, 2, -1, 2, 1, 2, 3, 3, 3, 3, 3,
    };
    static constexpr int NumVertices[NUM_TYPES] = {
        0, 1, -1, 2, -1, 3, -1, -1, -1, 4, 4, -1, 8, 6, 5,
    };
    static constexpr int NumFaces[NUM_TYPES] = {
        0, 0, -1, 0, -1, 1, -1, 1, -1, 1, 4, -1, 6, 5, 5,
    };
    static constexpr unsigned FaceSizes[NUM_TYPES][MaxNumFaces] = {
        // none
        {0, 0, 0, 0, 0, 0},
        // point
        {0, 0, 0, 0, 0, 0},
        // invalid
        {0, 0, 0, 0, 0, 0},
        // bar
        {0, 0, 0, 0, 0, 0},
        // invalid
        {0, 0, 0, 0, 0, 0},
        // triangle
        {3, 0, 0, 0, 0, 0},
        // invalid
        {0, 0, 0, 0, 0, 0},
        // polygon
        {0, 0, 0, 0, 0, 0},
        // invalid
        {0, 0, 0, 0, 0, 0},
        // quad
        {4, 0, 0, 0, 0, 0},
        // tetrahedron
        {3, 3, 3, 3, 0, 0},
        // polyhedron
        {0, 0, 0, 0, 0, 0},
        // hexahedron
        {4, 4, 4, 4, 4, 4},
        // prism
        {3, 4, 4, 4, 3, 0},
        // pyramid
        {4, 3, 3, 3, 3, 0},
    };
    static constexpr unsigned FaceVertices[NUM_TYPES][MaxNumFaces][MaxNumVertices] = {
        // clang-format off
    
        { // none
        },
        { // point
        },
        { // none
        },
        { // bar
        },
        { // none
        },
        { // triangle
        { 2, 1, 0 },
        },
        { // none
        },
        { // polygon
        },
        { // none
        },
        { // quad
        { 3, 2, 1, 0 },
        },
        { // tetrahedron
            /*
                    3
                   /|\
                  / | \
                 /  |  \
                /   2   \
               /  /   \  \
              / /       \ \
             //           \\
             0---------------1
            */
            { 2, 1, 0 },
            { 0, 1, 3 },
            { 1, 2, 3 },
            { 2, 0, 3 },
        },
        { // polyhedron
        },
        { // hexahedron
            /*
                   7 -------- 6
                  /|         /|
                 / |        / |
                4 -------- 5  |
                |  3-------|--2
                | /        | /
                |/         |/
                0----------1
            */
            { 3, 2, 1, 0 },
            { 4, 5, 6, 7 },
            { 0, 1, 5, 4 },
            { 7, 6, 2, 3 },
            { 1, 2, 6, 5 },
            { 4, 7, 3, 0 },
        },
        { // prism
            /*
                      5
                    / | \
                  /   |   \
                3 -------- 4
                |     2    |
                |   /   \  |
                | /       \|
                0----------1
            */
            {2, 1, 0},
            {0, 1, 4, 3},
            {1, 2, 5, 4},
            {2, 0, 3, 5},
            {3, 4, 5}
        },
        { // pyramid
            /*
                        4
                    / /  \    \
                  0--/------\-------1
                 / /          \    /
                //              \ /
               3-----------------2
            */
            { 3, 2, 1, 0 },
            { 0, 1, 4 },
            { 1, 2, 4 },
            { 2, 3, 4 },
            { 3, 0, 4 },
        },
        };
    // clang-format on


    UnstructuredGrid(const size_t numElements, const size_t numCorners, const size_t numVertices,
                     const Meta &meta = Meta());

    void resetElements() override;

    shm<Byte>::array &tl() { return *d()->tl; }
    const ShmArrayProxy<Byte> &tl() const { return m_tl; }
    shm<Byte>::array &isGhost() { return *d()->isGhost; }
    const ShmArrayProxy<Byte> &isGhost() const { return m_isGhost; }

    void setIsGhost(Index index, bool setTo);
    bool getIsGhost(Index index) const;


    bool isConvex(Index elem) const;
    bool isGhostCell(Index elem) const override;
    std::pair<Vector3, Vector3> cellBounds(Index elem) const override;
    Index findCell(const Vector3 &point, Index hint = InvalidIndex, int flags = NoFlags) const override;
    bool insideConvex(Index elem, const Vector3 &point) const;
    bool inside(Index elem, const Vector3 &point) const override;
    Index checkConvexity(); //< return number of non-convex cells
    Scalar exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const override;

    Interpolator getInterpolator(Index elem, const Vector3 &point, Mapping mapping = Vertex,
                                 InterpolationMode mode = Linear) const override;
    std::pair<Vector3, Vector3> elementBounds(Index elem) const override;
    std::vector<Index> cellVertices(Index elem) const override;
    Scalar cellDiameter(Index elem) const override;
    Vector3 cellCenter(Index elem) const override;
    std::vector<Index> getNeighborElements(Index elem) const override;
    Index cellNumFaces(Index elem) const override;

private:
    mutable ShmArrayProxy<Byte> m_tl;
    mutable ShmArrayProxy<Byte> m_isGhost;

    V_DATA_BEGIN(UnstructuredGrid);
    ShmVector<Byte> tl;
    ShmVector<Byte> isGhost;

    Data(const size_t numElements = 0, const size_t numCorners = 0, const size_t numVertices = 0,
         const std::string &name = "", const Meta &meta = Meta());
    static Data *create(const size_t numElements = 0, const size_t numCorners = 0, const size_t numVertices = 0,
                        const Meta &meta = Meta());

    V_DATA_END(UnstructuredGrid);
};

} // namespace vistle
#endif
