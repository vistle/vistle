#ifndef CELLTREE_IMPL_H
#define CELLTREE_IMPL_H

namespace vistle {

//#define CT_DEBUG
//#define CT_DEBUG_VERBOSE

//const Scalar Epsilon = std::numeric_limits<Scalar>::epsilon();

const unsigned MaxLeafSize = 8;

template<typename Scalar, typename Index, int NumDimensions>
Object::Type Celltree<Scalar, Index, NumDimensions>::type()
{
    return (Object::Type)(Object::CELLTREE1 + NumDimensions - 1);
}

template<typename Scalar, typename Index, int NumDimensions>
V_COREEXPORT void Celltree<Scalar, Index, NumDimensions>::refreshImpl() const
{}

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::init(const CTVector *min, const CTVector *max, const CTVector &gmin,
                                                  const CTVector &gmax)
{
    assert(nodes().size() == 1);
    for (int i = 0; i < NumDimensions; ++i)
        this->min()[i] = gmin[i];
    for (int i = 0; i < NumDimensions; ++i)
        this->max()[i] = gmax[i];
#ifdef CT_DEBUG
    struct MinMaxBoundsFunctor: public Celltree::CellBoundsFunctor {
        const CTVector *m_min, *m_max;

        MinMaxBoundsFunctor(const CTVector *min, const CTVector *max): m_min(min), m_max(max) {}

        bool operator()(Index elem, CTVector *min, CTVector *max) const
        {
            auto vmin = m_min[elem], vmax = m_max[elem];
            *min = vmin;
            *max = vmax;

            return true;
        }
    };
    MinMaxBoundsFunctor boundFunc(min, max);
    this->validateTree(boundFunc);
#endif
    refine(min, max, 0, gmin, gmax);
#ifdef CT_DEBUG
    std::cerr << "created celltree: " << nodes().size() << " nodes, " << cells().size() << " cells" << std::endl;
    validateTree(boundFunc);
#endif
}

template<typename Scalar, typename Index, int NumDimensions>
void Celltree<Scalar, Index, NumDimensions>::refine(const CTVector *min, const CTVector *max, Index curNode,
                                                    const CTVector &gmin, const CTVector &gmax)
{
    const Scalar smax = std::numeric_limits<Scalar>::max();

    const int NumBuckets = 5;

    Node *node = &(nodes()[curNode]);

    // only split node if necessary
    if (node->size <= MaxLeafSize)
        return;

    // cell index array, contains runs of cells belonging to nodes
    Index *cells = d()->m_cells->data();

    // sort cells into buckets for each possible split dimension

    // initialize min/max extents of buckets
    Index bucket[NumBuckets][NumDimensions];
    CTVector bmin[NumBuckets], bmax[NumBuckets];
    for (int i = 0; i < NumBuckets; ++i) {
        for (int d = 0; d < NumDimensions; ++d)
            bucket[i][d] = 0;
        bmin[i].fill(smax);
        bmax[i].fill(-smax);
    }

    auto center = [min, max](Index c) -> CTVector {
        return Scalar(0.5) * (min[c] + max[c]);
    };

    // find min/max extents of cell centers
    CTVector cmin, cmax;
    cmin.fill(smax);
    cmax.fill(-smax);
    for (Index i = node->start; i < node->start + node->size; ++i) {
        const Index cell = cells[i];
        CTVector cent = center(cell);
        for (int d = 0; d < NumDimensions; ++d) {
            if (cmin[d] > cent[d])
                cmin[d] = cent[d];
            if (cmax[d] < cent[d])
                cmax[d] = cent[d];
        }
    }

    // sort cells into buckets
    const CTVector crange = cmax - cmin;

    auto getBucket = [cmin, cmax, crange, NumBuckets](Scalar center, int d) -> int {
        return crange[d] == 0 ? 0 : std::min(int((center - cmin[d]) / crange[d] * NumBuckets), NumBuckets - 1);
    };

    for (Index i = node->start; i < node->start + node->size; ++i) {
        const Index cell = cells[i];
        const CTVector cent = center(cell);
        for (int d = 0; d < NumDimensions; ++d) {
            const int b = getBucket(cent[d], d);
            assert(b >= 0);
            assert(b < NumBuckets);
            ++bucket[b][d];
            bmin[b][d] = std::min(bmin[b][d], min[cell][d]);
            bmax[b][d] = std::max(bmax[b][d], max[cell][d]);
        }
    }

    // adjust bucket bounds for empty buckets
    for (int d = 0; d < NumDimensions; ++d) {
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
        Index nleft = 0;
        for (int split_b = 0; split_b < NumBuckets - 1; ++split_b) {
            nleft += bucket[split_b][d];
            assert(node->size >= nleft);
            const Index nright = node->size - nleft;
            Scalar weight =
                nleft * (bmax[split_b][d] - bmin[0][d]) + nright * (bmax[NumBuckets - 1][d] - bmin[split_b + 1][d]);
            if (crange[d] > 0)
                weight /= node->size * crange[d];
            else
                weight = smax;
            //std::cerr << "d="<<d<< ", b=" << split_b<< ", weight=" << weight << std::endl;
            if (nleft > 0 && nright > 0 && weight < min_weight) {
                min_weight = weight;
                best_dim = d;
                best_bucket = split_b;
            }
        }
        assert(nleft + bucket[NumBuckets - 1][d] == node->size);
    }
    if (best_dim == -1) {
        std::cerr << "abandoning split with " << node->size << " children" << std::endl;
        return;
    }

    // split index lists...
    const Index size = node->size;
    const Index start = node->start;

    // record children into node being split
    const Scalar Lmax = bmax[best_bucket][best_dim];
    const Scalar Rmin = bmin[best_bucket + 1][best_dim];
    *node = Node(best_dim, Lmax, Rmin, nodes().size());
    const Index D = best_dim;

    auto centerD = [min, max, D](Index c) -> Scalar {
        return Scalar(0.5) * (min[c][D] + max[c][D]);
    };

#ifdef CT_DEBUG
    const Scalar split = cmin[D] + crange[D] / NumBuckets * (best_bucket + 1);
#ifdef CT_DEBUG_VERBOSE
    std::cerr << "split: dim=" << best_dim << ", bucket=" << best_bucket;
    std::cerr << " (";
    for (int i = 0; i < NumBuckets; ++i) {
        std::cerr << bucket[i][best_dim];
        if (i == best_bucket)
            std::cerr << "|";
        else if (i < NumBuckets - 1)
            std::cerr << " ";
    }
    std::cerr << ")";
    std::cerr << " split: " << split;
    std::cerr << " (" << cmin[D] << " - " << cmax[D] << ")";
    std::cerr << std::endl;
#endif
#endif

    Index nleft = 0;
    Index *top = &cells[start + size - 1];
    for (Index *c = &cells[start]; c <= top; ++c) {
        const Scalar cent = centerD(*c);
        const int b = getBucket(cent, D);
        if (b <= best_bucket) {
            ++nleft;
            continue;
        }
        for (; c < top; --top) {
            Scalar other = centerD(*top);
            const int bo = getBucket(other, D);
            if (bo <= best_bucket) {
                std::swap(*c, *top);
                ++nleft;
                break;
            }
        }
    }

#ifdef CT_DEBUG
    for (Index i = nleft; i < size; ++i) {
        Index c = cells[start + i];
        CTVector cent = center(c);
        assert(cent[D] >= split);
        assert(min[c][D] >= Rmin);
        for (int i = 0; i < 3; ++i) {
            assert(min[c][i] >= gmin[i]);
            assert(max[c][i] <= gmax[i]);
        }
    }
    for (Index i = 0; i < nleft; ++i) {
        Index c = cells[start + i];
        CTVector cent = center(c);
        assert(cent[D] <= split);
        assert(max[c][D] <= Lmax);
        for (int i = 0; i < 3; ++i) {
            assert(min[c][i] >= gmin[i]);
            assert(max[c][i] <= gmax[i]);
        }
    }
#endif

    Index l = nodes().size();
    nodes().push_back(Node(start, nleft));

    Index r = nodes().size();
    nodes().push_back(Node(start + nleft, size - nleft));

    assert(nodes()[l].size < size);
    assert(nodes()[r].size < size);
    assert(nodes()[l].size + nodes()[r].size == size);

    CTVector nmin = gmin;
    CTVector nmax = gmax;

    // further refinement for left...
    nmin[best_dim] = bmin[0][best_dim];
    nmax[best_dim] = bmax[best_bucket][best_dim];
    refine(min, max, l, nmin, nmax);

    // ...and right subnodes
    nmin[best_dim] = bmin[best_bucket + 1][best_dim];
    nmax[best_dim] = bmax[NumBuckets - 1][best_dim];
    refine(min, max, r, nmin, nmax);
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
Celltree<Scalar, Index, NumDimensions>::Celltree(const Index numCells, const Meta &meta)
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
bool Celltree<Scalar, Index, NumDimensions>::checkImpl() const
{
    V_CHECK(d()->m_nodes->size() >= 1);
    V_CHECK(d()->m_nodes->size() <= d()->m_cells->size());
    if ((*d()->m_nodes)[0].isLeaf()) {
        V_CHECK((*d()->m_nodes)[0].size <= d()->m_cells->size());
    } else {
        V_CHECK((*d()->m_nodes)[0].Lmax >= (*d()->m_nodes)[0].Rmin);
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
Celltree<Scalar, Index, NumDimensions>::Data::Data(const std::string &name, const Index numCells, const Meta &meta)
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
Celltree<Scalar, Index, NumDimensions>::Data::create(const std::string &objId, const Index numCells, const Meta &meta)
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
