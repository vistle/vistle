#ifndef CELLTREE_IMPL_H
#define CELLTREE_IMPL_H

#include "shm_array_impl.h"
#include "vector.h"
#include <deque>

#define CT_PARALLEL_BUILD
//#define CT_DEBUG


#ifdef CT_PARALLEL_BUILD
#include <mutex>
#include <thread>
#endif

#include "validate.h"

namespace vistle {

namespace {
const int NumBuckets = 7;
const unsigned MaxLeafSize = 32;
const float FavorEqualSplits = 2.f;
} // namespace

template<typename Scalar, typename Index, int NumDimensions>
struct Celltree<Scalar, Index, NumDimensions>::NodeData {
    NodeData(Index node, Index depth, Index start, Index size): node(node), depth(depth), start(start), size(size) {}
    NodeData(const NodeData &parent, Index node, Index start, Index size)
    : node(node), depth(parent.depth + 1), start(start), size(size)
    {}
    Index node = InvalidIndex;
    Index depth = 0;
    Index start = InvalidIndex;
    Index size = 0;
};

template<typename Scalar, typename Index, int NumDimensions>
struct Celltree<Scalar, Index, NumDimensions>::GlobalData {
#ifdef CT_PARALLEL_BUILD
    std::mutex mutex;
    std::atomic<unsigned> numRunning = 0;
    std::atomic<unsigned> numWorking = 0;
#endif
    std::deque<NodeData> nodesToSplit;
};

template<typename Scalar, typename Index, int NumDimensions>
Object::Type Celltree<Scalar, Index, NumDimensions>::type()
{
    return (Object::Type)(Object::CELLTREE1 + NumDimensions - 1);
}

template<typename Scalar, typename Index, int NumDimensions>
V_COREEXPORT void Celltree<Scalar, Index, NumDimensions>::refreshImpl() const
{}

template<typename Scalar, typename Index, int NumDimensions>
V_COREEXPORT void Celltree<Scalar, Index, NumDimensions>::print(std::ostream &os, bool verbose) const
{
    Base::print(os);
    os << " cells:";
    d()->m_cells.print(os, verbose);

    os << " nodes:";
    d()->m_nodes.print(os, verbose);
}

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::init(const AABB *bounds, const CTVector &gmin, const CTVector &gmax)
{
    assert(nodes().size() == 1);
    for (int i = 0; i < NumDimensions; ++i)
        this->min()[i] = gmin[i];
    for (int i = 0; i < NumDimensions; ++i)
        this->max()[i] = gmax[i];
    nodes().reserve(cells().size() / MaxLeafSize);
    GlobalData data;
    NodeData node(0, 0, nodes()[0].start, nodes()[0].size);
    data.nodesToSplit.emplace_front(node);

    size_t nthreads = 1;
#ifdef CT_PARALLEL_BUILD
    std::deque<std::thread> threads;
    std::unique_lock guard(data.mutex);
    while (data.numWorking > 0 || !data.nodesToSplit.empty()) {
        guard.unlock();
        if (data.numRunning < (std::thread::hardware_concurrency() + 1) / 2) {
            threads.push_back(std::thread([this, &bounds, &data]() mutable {
                static const unsigned maxidlecount = 5;
                unsigned idlecount = 0;
                ++data.numRunning;
                do {
                    std::unique_lock guard(data.mutex);
                    while (!data.nodesToSplit.empty()) {
                        idlecount = 0;
                        auto n = data.nodesToSplit.front();
                        data.nodesToSplit.pop_front();
                        ++data.numWorking;
                        guard.unlock();
                        refine(bounds, n, data);
                        guard.lock();
                        --data.numWorking;
                    }
                    guard.unlock();
                    ++idlecount;
                    if (data.numWorking == 0)
                        continue;
                    usleep(10000 * idlecount);
                } while (idlecount < maxidlecount && data.numWorking > 0);
                --data.numRunning;
            }));
        }
        usleep(10000);
        guard.lock();
    }
    guard.unlock();
    nthreads += threads.size();
    while (!threads.empty()) {
        threads.front().join();
        threads.pop_front();
    }
#else
    while (!data.nodesToSplit.empty()) {
        auto n = data.nodesToSplit.front();
        data.nodesToSplit.pop_front();
        refine(bounds, n, data);
    }
#endif

    std::cerr << "created celltree: " << nodes().size() << " nodes, " << cells().size() << " cells, used " << nthreads
              << " threads" << std::endl;
}

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::refine(const AABB *bounds, Celltree::NodeData &nodeData,
                                                    Celltree::GlobalData &data)
{
    static const Scalar smax = std::numeric_limits<Scalar>::max();

    const Index nodeStart = nodeData.start;
    const Index nodeSize = nodeData.size;

    // only split node if necessary
    if (nodeSize <= MaxLeafSize)
        return;

    // cell index array, contains runs of cells belonging to nodes
    Index *cells = d()->m_cells->data();

    // sort cells into buckets for each possible split dimension
    auto center = [&bounds](int d, Index cell) {
        const auto &c = bounds[cell];
        return Scalar(0.5) * (c.min(d) + c.max(d));
    };

    // find min/max extents of cell centers
    CTVector cmin, cmax;
    cmin.fill(smax);
    cmax.fill(-smax);
    for (Index i = nodeStart; i < nodeStart + nodeSize; ++i) {
        const Index cell = cells[i];
        for (int d = 0; d < NumDimensions; ++d) {
            const auto cent = center(d, cell);
            cmin[d] = std::min(cmin[d], cent);
            cmax[d] = std::max(cmax[d], cent);
        }
    }

    // sort cells into buckets
    const CTVector crange = (cmax - cmin);
    const CTVector crangeI = crange.cwiseInverse() * NumBuckets;
    auto getBucket = [cmin, cmax, crangeI](Scalar center, int d) -> int {
        return std::min(int((center - cmin[d]) * crangeI[d]), NumBuckets - 1);
    };

    // initialize min/max extents of buckets
    Index bucket[NumBuckets][NumDimensions];
    CTVector bmin[NumBuckets]{CTVector::Constant(smax)}, bmax[NumBuckets]{CTVector::Constant(-smax)};
    for (int i = 0; i < NumBuckets; ++i) {
        for (int d = 0; d < NumDimensions; ++d)
            bucket[i][d] = 0;
        bmin[i].fill(smax);
        bmax[i].fill(-smax);
    }

    for (Index i = nodeStart; i < nodeStart + nodeSize; ++i) {
        const Index cell = cells[i];
        for (int d = 0; d < NumDimensions; ++d) {
            if (crange[d] == 0)
                continue;
            const int b = getBucket(center(d, cell), d);
            assert(b >= 0);
            assert(b < NumBuckets);
            ++bucket[b][d];
            const auto &bound = bounds[cell];
            bmin[b][d] = std::min(bmin[b][d], bound.min(d));
            bmax[b][d] = std::max(bmax[b][d], bound.max(d));
        }
    }

    // adjust bucket bounds for empty buckets
    for (int d = 0; d < NumDimensions; ++d) {
        if (crange[d] == 0)
            continue;
        for (int b = NumBuckets - 2; b >= 0; --b) {
            if (bmin[b][d] > bmin[b + 1][d])
                bmin[b][d] = bmin[b + 1][d];
        }
        for (int b = 1; b < NumBuckets; ++b) {
            if (bmax[b][d] < bmax[b - 1][d])
                bmax[b][d] = bmax[b - 1][d];
        }
    }

    // find best split dimension and plane
    Scalar min_weight(smax);
    int best_dim = -1, best_bucket = -1;
    for (int d = 0; d < NumDimensions; ++d) {
        if (crange[d] == 0)
            continue;
        Index nleft = 0;
        for (int split_b = 0; split_b < NumBuckets - 1; ++split_b) {
            nleft += bucket[split_b][d];
            assert(nodeSize >= nleft);
            const Index nright = nodeSize - nleft;
            Scalar weight =
                std::pow(float(nleft) / float(nodeSize), FavorEqualSplits) * (bmax[split_b][d] - bmin[0][d]) +
                std::pow(float(nright) / float(nodeSize), FavorEqualSplits) *
                    (bmax[NumBuckets - 1][d] - bmin[split_b + 1][d]);
            weight /= crange[d];
            //std::cerr << "d=" << d << ", b=" << split_b << ", weight=" << weight << std::endl;
            if (nleft > 0 && nright > 0 && weight < min_weight) {
                min_weight = weight;
                best_dim = d;
                best_bucket = split_b;
            }
        }
        assert(nleft + bucket[NumBuckets - 1][d] == nodeSize);
    }

    // split index lists...
    const Index size = nodeSize;
    const Index start = nodeStart;

    Index nleft = 0;
    Scalar Lmax, Rmin;
    if (best_dim == -1) {
        crange.maxCoeff(&best_dim);
        nleft = size / 2;
        std::cerr << "abandoning split with " << nodeSize << " children, fall back to @" << best_dim << " after "
                  << nleft << " cells" << std::endl;
        std::nth_element(&cells[start], &cells[start + nleft], &cells[start + size],
                         [center, best_dim](Index a, Index b) { return center(best_dim, a) < center(best_dim, b); });

        auto findBounds = [bounds, cells](Index begin, Index end) -> std::pair<CTVector, CTVector> {
            CTVector gmin, gmax;
            gmin.fill(smax);
            gmax.fill(-smax);
            for (Index i = begin; i < end; ++i) {
                const Index cell = cells[i];
                const auto &bound = bounds[cell];
                for (int d = 0; d < NumDimensions; ++d) {
                    gmin[d] = std::min(gmin[d], bound.min(d));
                    gmax[d] = std::max(gmax[d], bound.max(d));
                }
            }
            return std::make_pair(gmin, gmax);
        };

        auto Lbounds = findBounds(start, start + nleft);
        auto Rbounds = findBounds(start + nleft, start + size);
        auto Lmax = Lbounds.second[best_dim];
        auto Rmin = Rbounds.first[best_dim];

#ifdef CT_PARALLEL_BUILD
        std::unique_lock guard(data.mutex);
#endif
        Node *node = &(nodes()[nodeData.node]);
        // promote to inner node
        *node = Node(best_dim, Lmax, Rmin, nodes().size());
        Index l = nodes().size();
        nodes().push_back(Node(start, nleft));

        Index r = nodes().size();
        nodes().push_back(Node(start + nleft, size - nleft));

        assert(nodes()[l].size < size);
        assert(nodes()[r].size < size);
        assert(nodes()[l].size + nodes()[r].size == size);

        data.nodesToSplit.emplace_front(nodeData, l, start, nleft);
        data.nodesToSplit.emplace_front(nodeData, r, start + nleft, size - nleft);
        return;
    } else {
        auto mid =
            std::partition(&cells[start], &cells[start + size], [getBucket, center, best_dim, best_bucket](Index c) {
                return getBucket(center(best_dim, c), best_dim) <= best_bucket;
            });
        nleft = mid - &cells[start];
        Lmax = bmax[best_bucket][best_dim];
        Rmin = bmin[best_bucket + 1][best_dim];
    }

    // record children into node being split

#ifdef CT_PARALLEL_BUILD
    std::unique_lock guard(data.mutex);
#endif
    Node *node = &(nodes()[nodeData.node]);
    // promote to inner node
    *node = Node(best_dim, Lmax, Rmin, nodes().size());
    Index l = nodes().size();
    nodes().push_back(Node(start, nleft));

    Index r = nodes().size();
    nodes().push_back(Node(start + nleft, size - nleft));

    assert(nodes()[l].size < size);
    assert(nodes()[r].size < size);
    assert(nodes()[l].size + nodes()[r].size == size);

    data.nodesToSplit.emplace_front(nodeData, l, start, nleft);
    data.nodesToSplit.emplace_front(nodeData, r, start + nleft, size - nleft);
}

template<typename Scalar, typename Index, int NumDimensions>
template<class BoundsFunctor>
bool Celltree<Scalar, Index, NumDimensions>::validateTree(BoundsFunctor &boundFunc) const
{
    CTVector mmin, mmax;
    for (int c = 0; c < NumDimensions; ++c) {
        mmin[c] = min()[c];
        mmax[c] = max()[c];
    }
#ifdef CT_DEBUG
    std::cerr << "validateCelltree: min=" << mmin << ", max=" << mmax << std::endl;
#endif
    return validateNode(boundFunc, 0, mmin, mmax);
}

template<typename Scalar, typename Index, int NumDimensions>
template<class BoundsFunctor>
bool Celltree<Scalar, Index, NumDimensions>::validateNode(BoundsFunctor &boundFunc, Index nodenum, const CTVector &min,
                                                          const CTVector &max) const
{
    bool valid = true;
    const Index *cells = d()->m_cells->data();
    const Node *node = &(nodes()[nodenum]);
    if (node->isLeaf()) {
        for (Index i = node->start; i < node->start + node->size; ++i) {
            const Index elem = cells[i];
            CTVector emin, emax;
            boundFunc(elem, &emin, &emax);
            for (int d = 0; d < NumDimensions; ++d) {
                if (emin[d] < min[d]) {
                    std::cerr << "celltree node " << nodenum << " @" << i - node->start << " of " << node->size
                              << ": min[" << d << "] violation on elem " << elem << ": is " << emin << ", should "
                              << min << std::endl;
                    valid = false;
                }
                if (emax[d] > max[d]) {
                    std::cerr << "celltree node " << nodenum << " @" << i - node->start << " of " << node->size
                              << ": max[" << d << "] violation on elem " << elem << ": is " << emax << ", should "
                              << max << std::endl;
                    valid = false;
                }
            }
        }
        return valid;
    }

    if (node->Lmax < min[node->dim]) {
        std::cerr << "celltree: Lmax=" << node->Lmax << " too small for dim=" << node->dim
                  << ", should >= " << min[node->dim] << std::endl;
        valid = false;
    }
    if (node->Rmin < min[node->dim]) {
        std::cerr << "celltree: Rmin=" << node->Rmin << " too small for dim=" << node->dim
                  << ", should >= " << min[node->dim] << std::endl;
        valid = false;
    }
    if (node->Lmax > max[node->dim]) {
        std::cerr << "celltree: Lmax=" << node->Lmax << " too large for dim=" << node->dim
                  << ", should <= " << max[node->dim] << std::endl;
        valid = false;
    }
    if (node->Rmin > max[node->dim]) {
        std::cerr << "celltree: Rmin=" << node->Rmin << " too large for dim=" << node->dim
                  << ", should <= " << max[node->dim] << std::endl;
        valid = false;
    }

    CTVector lmax(max), rmin(min);
    lmax[node->dim] = node->Lmax;
    rmin[node->dim] = node->Rmin;

    if (!validateNode(boundFunc, node->left(), min, lmax))
        valid = false;
    if (!validateNode(boundFunc, node->right(), rmin, max))
        valid = false;
    return valid;
}

template<class Scalar, class Index, int NumDimensions>
template<class Archive>
void Celltree<Scalar, Index, NumDimensions>::Data::serialize(Archive &ar)
{
    assert("serialization of Celltree::Node not implemented" == 0);
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Celltree(const size_t numCells, const Meta &meta)
: Celltree::Base(Celltree::Data::create("", numCells, meta))
{
    refreshImpl();
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Celltree(): Celltree::Base()
{
    refreshImpl();
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Celltree(Data *data): Celltree::Base(data)
{
    refreshImpl();
}

template<typename Scalar, typename Index, int NumDimensions>
bool Celltree<Scalar, Index, NumDimensions>::isEmpty() const
{
    return d()->m_nodes->empty() || d()->m_cells->empty();
}

template<typename Scalar, typename Index, int NumDimensions>
bool Celltree<Scalar, Index, NumDimensions>::isEmpty()
{
    return d()->m_nodes->empty() || d()->m_cells->empty();
}

template<typename Scalar, typename Index, int NumDimensions>
bool Celltree<Scalar, Index, NumDimensions>::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_INDEX(d()->m_nodes->size());
    VALIDATE_INDEX(d()->m_cells->size());

    VALIDATE(d()->m_nodes->size() >= 1);
    VALIDATE(d()->m_nodes->size() <= d()->m_cells->size());
    if ((*d()->m_nodes)[0].isLeaf()) {
        VALIDATE((*d()->m_nodes)[0].size <= d()->m_cells->size());
    } else {
        VALIDATE((*d()->m_nodes)[0].Lmax >= (*d()->m_nodes)[0].Rmin);
    }
    return true;
}

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::Data::initData()
{
    const Scalar smax = std::numeric_limits<Scalar>::max();
    for (int d = 0; d < NumDimensions; ++d) {
        m_bounds[d] = smax;
        m_bounds[NumDimensions + d] = -smax;
    }

    m_nodes.construct(1);
    (*m_nodes)[0] = Node(0, 0);
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Data::Data(const Data &o, const std::string &n)
: Celltree::Base::Data(o, n), m_cells(o.m_cells), m_nodes(o.m_nodes)
{
    memcpy(m_bounds, o.m_bounds, sizeof(m_bounds));
}

template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Data::Data(const std::string &name, const size_t numCells, const Meta &meta)
: Base::Data(Object::Type(Object::CELLTREE1 - 1 + NumDimensions), name, meta)
{
    initData();

    m_cells.construct(numCells);

    Index *cells = m_cells->data();
    for (Index i = 0; i < numCells; ++i) {
        cells[i] = i;
    }

    (*m_nodes)[0] = Node(0, numCells);
}

template<typename Scalar, typename Index, int NumDimensions>
typename Celltree<Scalar, Index, NumDimensions>::Data *
Celltree<Scalar, Index, NumDimensions>::Data::create(const std::string &objId, const size_t numCells, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId(objId);
    Data *ct = shm<Data>::construct(name)(name, numCells, meta);
    publish(ct);

    return ct;
}

#if 0
V_OBJECT_CREATE_NAMED(Celltree<Scalar, Index, NumDimensions>)
#else
template<typename Scalar, typename Index, int NumDimensions>
Celltree<Scalar, Index, NumDimensions>::Data::Data(Object::Type id, const std::string &name, const Meta &meta)
: Base::Data(id, name, meta)
{
    initData();
}

template<typename Scalar, typename Index, int NumDimensions>
typename Celltree<Scalar, Index, NumDimensions>::Data *
Celltree<Scalar, Index, NumDimensions>::Data::createNamed(Object::Type id, const std::string &name)
{
    Data *t = shm<Data>::construct(name)(id, name, Meta());
    publish(t);
    return t;
}
#endif

} // namespace vistle
#endif
