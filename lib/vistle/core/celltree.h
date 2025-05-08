#ifndef VISTLE_CORE_CELLTREE_H
#define VISTLE_CORE_CELLTREE_H

#include "export.h"
#include "scalar.h"
#include "index.h"
#include "shm.h"
#include "object.h"
#include "vectortypes.h"
#include "geometry.h"
#include "shmvector.h"

#include "celltreenode_decl.h"

namespace vistle {

// a bounding volume hierarchy, cf. C. Garth and K. I. Joy:
// “Fast, memory-efficient cell location in unstructured grids for visualization”,
// IEEE Transactions on Visualization and Computer Graphics, vol. 16, no. 6, pp. 1541–1550, 2010.

template<typename Scalar, typename Index, int NumDimensions = 3>
class V_COREEXPORT Celltree: public Object {
    V_OBJECT(Celltree);

public:
    typedef Object Base;

    typedef VistleVector<Scalar, NumDimensions> CTVector;
    typedef CelltreeNode<Scalar, sizeof(Index), NumDimensions> Node;

    struct AABB {
        Scalar mmin[NumDimensions];
        Scalar mmax[NumDimensions];

        Scalar min(int d) const { return mmin[d]; }
        Scalar max(int d) const { return mmax[d]; }
    };

    class VisitFunctor {
    public:
        enum Order {
            None = 0, //< none of the subnodes have to be visited
            RightFirst = 1, //< right subnode has to be visited, should be visited first
            RightSecond = 2, //< right subnode has to be visited, should be visited last
            Left = 4, //< left subnode has to be visited
            Right = RightFirst, //< right subnode has to be visited
            LeftRight = Left | RightSecond, //< both subnodes have to be visited, preferably left first
            RightLeft = Left | RightFirst, //< both subnodes have to be visited, preferably right first
        };

        //! check whether the celltree is within bounds min and max, otherwise no traversal
        bool checkBounds(const Scalar *min, const Scalar *max)
        {
            (void)min;
            (void)max;
            return true; // continue traversal
        }

        //! return whether and in which order to visit children of a node
        Order operator()(const Node &node)
        {
            (void)node;
            return LeftRight;
        }
    };

    //! return whether further cells have to be visited
    class LeafFunctor {
    public:
        bool operator()(Index elem)
        {
            return true; // continue traversal
        }
    };

    //! compute bounding box of a cell
    class CellBoundsFunctor {
    public:
        bool operator()(Index elem, CTVector *min, CTVector *max)
        {
            return false; // couldn't compute bounds
        }
    };

    Celltree(const size_t numCells, const Meta &meta = Meta());

    void init(const AABB *bounds, const CTVector &gmin, const CTVector &gmax);
    template<class BoundsFunctor>
    bool validateTree(BoundsFunctor &func) const;

    Scalar *min() { return &(d()->m_bounds)[0]; }
    const Scalar *min() const { return &(d()->m_bounds)[0]; }
    Scalar *max() { return &(d()->m_bounds)[NumDimensions]; }
    const Scalar *max() const { return &(d()->m_bounds)[NumDimensions]; }
    typename shm<Node>::array &nodes() { return *d()->m_nodes; }
    const typename shm<Node>::array &nodes() const { return *d()->m_nodes; }
    typename shm<Index>::array &cells() { return *d()->m_cells; }
    const typename shm<Index>::array &cells() const { return *d()->m_cells; }

    template<class InnerNodeFunctor, class ElementFunctor>
    void traverse(InnerNodeFunctor &visitNode, ElementFunctor &visitElement) const
    {
        if (!visitNode.checkBounds(min(), max()))
            return;
        traverseNode(0, nodes().data(), cells().data(), visitNode, visitElement);
    }

private:
    struct GlobalData;
    struct NodeData;
    void refine(const AABB *bounds, NodeData &node, GlobalData &data);

    template<class BoundsFunctor>
    bool validateNode(BoundsFunctor &func, Index nodenum, const CTVector &min, const CTVector &max) const;
    template<class InnerNodeFunctor, class ElementFunctor>
    bool traverseNode(Index curNode, const Node *nodes, const Index *cells, InnerNodeFunctor &visitNode,
                      ElementFunctor &visitElement) const
    {
        const Node &node = nodes[curNode];
        if (node.isLeaf()) {
            for (Index i = node.start; i < node.start + node.size; ++i) {
                const Index cell = cells[i];
                if (!visitElement(cell))
                    return false;
            }
            return true;
        }

        const typename VisitFunctor::Order order = visitNode(node);
        assert(!((order & VisitFunctor::RightFirst) && (order & VisitFunctor::RightSecond)));
        bool continueTraversal = true;
        if (continueTraversal && (order & VisitFunctor::RightFirst)) {
            continueTraversal = traverseNode(node.child + 1, nodes, cells, visitNode, visitElement);
        }
        if (continueTraversal && (order & VisitFunctor::Left)) {
            continueTraversal = traverseNode(node.child, nodes, cells, visitNode, visitElement);
        }
        if (continueTraversal && (order & VisitFunctor::RightSecond)) {
            continueTraversal = traverseNode(node.child + 1, nodes, cells, visitNode, visitElement);
        }

        return continueTraversal;
    }

    template<class Functor>
    bool traverseNode(Index curNode, Functor &func) const;

    V_DATA_BEGIN(Celltree);
    Scalar m_bounds[NumDimensions * 2];
    ShmVector<Index> m_cells;
    ShmVector<Node> m_nodes;

    static Data *create(const std::string &name = "", const size_t numCells = 0, const Meta &m = Meta());
    Data(const std::string &name = "", const size_t numCells = 0, const Meta &m = Meta());
    V_DATA_END(Celltree);
};

#define TD_CT(ST, SN, IS) \
    typedef Celltree<ST, Index##IS, 1> Celltree##IS##SN##1; \
    typedef Celltree<ST, Index##IS, 2> Celltree##IS##SN##2; \
    typedef Celltree<ST, Index##IS, 3> Celltree##IS##SN##3;

TD_CT(float, F, 32)
TD_CT(float, F, 64)
TD_CT(double, D, 32)
TD_CT(double, D, 64)

#define V_CELLTREE_SI(S, I, EXT, EXP) \
    EXT template class EXP Celltree<S, I, 1>; \
    EXT template class EXP Celltree<S, I, 2>; \
    EXT template class EXP Celltree<S, I, 3>;

#define V_CELLTREE_I(I, EXT, EXP) V_CELLTREE_SI(float, I, EXT, EXP) V_CELLTREE_SI(double, I, EXT, EXP)
#define V_CELLTREE_DECL_I(I) V_CELLTREE_I(I, extern, V_COREEXPORT)

FOR_ALL_INDEX(V_CELLTREE_DECL_I)

#define FOR_ALL_CELLTREE_TYPES(MACRO) \
    MACRO(Celltree32F1) \
    MACRO(Celltree32F2) \
    MACRO(Celltree32F3) \
    MACRO(Celltree32D1) \
    MACRO(Celltree32D2) \
    MACRO(Celltree32D3) \
    MACRO(Celltree64F1) \
    MACRO(Celltree64F2) \
    MACRO(Celltree64F3) \
    MACRO(Celltree64D1) \
    MACRO(Celltree64D2) \
    MACRO(Celltree64D3)

#define V_DECLARE_CT(T) V_DECLARE_SHMREF(T::Node)

FOR_ALL_CELLTREE_TYPES(V_DECLARE_CT)

typedef Celltree<Scalar, Index, 1> Celltree1;
typedef Celltree<Scalar, Index, 2> Celltree2;
typedef Celltree<Scalar, Index, 3> Celltree3;

} // namespace vistle

#include "celltreenode.h"

namespace vistle {

template<int Dim>
class V_COREEXPORT CelltreeInterface: virtual public ElementInterface {
public:
    typedef vistle::Celltree<Scalar, Index, Dim> Celltree;
    virtual bool hasCelltree() const = 0;
    virtual typename Celltree::const_ptr getCelltree() const = 0;
    virtual bool validateCelltree() const = 0;
};

} // namespace vistle

// include only where actually required
//#include "celltree_impl.h"

#endif
