#include "ngons.h"
#include "triangles.h"
#include "quads.h"
#include "celltree_impl.h"
#include "ngons_impl.h"
#include "archives.h"
#include "celltypes.h"
#include <cassert>
#include <vistle/util/exception.h>
#include "validate.h"

namespace vistle {

template<int N>
V_OBJECT_IMPL_LOAD(Ngons<N>)
template<int N>
V_OBJECT_IMPL_SAVE(Ngons<N>)

template<int N>
Ngons<N>::Ngons(const size_t numCorners, const size_t numCoords, const Meta &meta)
: Ngons::Base(Ngons::Data::create(numCorners, numCoords, meta))
{
    refreshImpl();
}

template<int N>
void Ngons<N>::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);
    if (d) {
        m_cl = d->cl;
        m_ghost = d->ghost;

    } else {
        m_cl = nullptr;
        m_ghost = nullptr;
    }
    m_numCorners = (d && d->cl.valid()) ? d->cl->size() : 0;
    m_celltree = nullptr;
}

template<int N>
bool Ngons<N>::isEmpty()
{
    return getNumCoords() == 0;
}

template<int N>
bool Ngons<N>::isEmpty() const
{
    return getNumCoords() == 0;
}

template<int N>
bool Ngons<N>::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_INDEX(d()->cl->size());
    VALIDATE_INDEX(d()->ghost->size());

    VALIDATE(d()->cl->check(os));
    VALIDATE(d()->ghost->check(os));
    VALIDATE(d()->ghost->size() == 0 || d()->ghost->size() == getNumElements());
    if (getNumCorners() > 0) {
        VALIDATE(cl()[0] < getNumVertices());
        VALIDATE(cl()[getNumCorners() - 1] < getNumVertices());
        VALIDATE(getNumCorners() % N == 0);
    } else {
        VALIDATE(getNumCoords() % N == 0);
    }

    if (quick)
        return true;

    if (getNumCorners() > 0) {
        VALIDATE_RANGE(cl(), 0, getNumVertices() - 1);
    }

    if (hasCelltree()) {
        VALIDATE(validateCelltree());
    }

    return true;
}

template<int N>
std::pair<Vector3, Vector3> Ngons<N>::elementBounds(Index elem) const
{
    const Index *cl = nullptr;
    if (getNumCorners() > 0)
        cl = &this->cl()[0];
    const Index begin = elem * N, end = begin + N;
    const Scalar *x[3] = {&this->x()[0], &this->y()[0], &this->z()[0]};

    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 min(smax, smax, smax), max(-smax, -smax, -smax);
    for (Index i = begin; i < end; ++i) {
        Index v = i;
        if (cl)
            v = cl[i];
        for (int c = 0; c < 3; ++c) {
            min[c] = std::min(min[c], x[c][v]);
            max[c] = std::max(max[c], x[c][v]);
        }
    }
    return std::make_pair(min, max);
}

template<int N>
std::vector<Index> Ngons<N>::cellVertices(Index elem) const
{
    std::vector<Index> result;
    result.reserve(N);
    const Index *cl = nullptr;
    if (getNumCorners() > 0)
        cl = &this->cl()[0];
    const Index begin = elem * N, end = begin + N;
    for (Index i = begin; i < end; ++i) {
        Index v = i;
        if (cl)
            v = cl[i];
        result.emplace_back(v);
    }
    return result;
}

template<int N>
bool Ngons<N>::hasCelltree() const
{
    if (m_celltree)
        return true;

    return hasAttachment("celltree");
}

template<int N>
Ngons<N>::Celltree::const_ptr Ngons<N>::getCelltree() const
{
    if (m_celltree)
        return m_celltree;

    typename Data::mutex_lock_type lock(d()->attachment_mutex);
    if (!hasAttachment("celltree")) {
        refresh();
        const Index *corners = nullptr;
        if (getNumCorners() > 0)
            corners = &cl()[0];
        createCelltree(getNumElements(), corners);
    }

    m_celltree = Celltree::as(getAttachment("celltree"));
    assert(m_celltree);
    return m_celltree;
}

template<int N>
void Ngons<N>::createCelltree(Index nelem, const Index *cl) const
{
    if (hasCelltree())
        return;

    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 vmin, vmax;
    vmin.fill(-smax);
    vmax.fill(smax);

    std::vector<Celltree::AABB> bounds(nelem);

    const Scalar *coords[3] = {&x()[0], &y()[0], &z()[0]};
    Vector3 gmin = vmax, gmax = vmin;
    for (Index i = 0; i < nelem; ++i) {
        Scalar min[3]{smax, smax, smax};
        Scalar max[3]{-smax, -smax, -smax};
        const Index start = i * N, end = start + N;
        for (Index c = start; c < end; ++c) {
            Index v = c;
            if (cl)
                v = cl[c];
            for (int d = 0; d < 3; ++d) {
                min[d] = std::min(min[d], coords[d][v]);
                max[d] = std::max(max[d], coords[d][v]);
            }
        }
        auto &b = bounds[i];
        for (int d = 0; d < 3; ++d) {
            gmin[d] = std::min(gmin[d], min[d]);
            gmax[d] = std::max(gmax[d], max[d]);
            b.mmin[d] = min[d];
            b.mmax[d] = max[d];
        }
    }

    typename Celltree::ptr ct(new Celltree(nelem));
    ct->init(bounds.data(), gmin, gmax);
    addAttachment("celltree", ct);
#ifndef NDEBUG
    if (!validateCelltree()) {
        std::cerr << "ERROR: Celltree validation failed." << std::endl;
    }
#endif
}

template<int N>
struct CellBoundsFunctor: public Ngons<N>::Celltree::CellBoundsFunctor {
    CellBoundsFunctor(const Ngons<N> *ngons): m_ngons(ngons) {}

    bool operator()(Index elem, Vector3 *min, Vector3 *max) const
    {
        auto b = m_ngons->elementBounds(elem);
        *min = b.first;
        *max = b.second;
        return true;
    }

    const Ngons<N> *m_ngons;
};

template<int N>
bool Ngons<N>::validateCelltree() const
{
    if (!hasCelltree())
        return false;

    CellBoundsFunctor<N> boundFunc(this);
    auto ct = getCelltree();
    if (!ct->validateTree(boundFunc)) {
        std::cerr << "Ngons<N=" << N << ">: Celltree validation failed with " << getNumElements()
                  << " elements total, bounds: " << getBounds().first << "-" << getBounds().second << std::endl;
        return false;
    }
    return true;
}

template<int N>
void Ngons<N>::setGhost(Index index, bool isGhost)
{
    assert(index < getNumElements());
    if (this->d()->ghost->size() < getNumElements())
        this->d()->ghost->resize(getNumElements(), cell::NORMAL);
    this->d()->ghost->at(index) = isGhost ? cell::GHOST : cell::NORMAL;
    refreshImpl();
}

template<int N>
bool Ngons<N>::isGhost(Index index) const
{
    assert(index < getNumElements());
    if (index >= this->d()->ghost->size())
        return false;
    return (this->ghost())[index] == cell::GHOST;
}


template<int N>
void Ngons<N>::Data::initData()
{}

template<int N>
Ngons<N>::Data::Data(const Ngons::Data &o, const std::string &n): Ngons::Base::Data(o, n), cl(o.cl), ghost(o.ghost)
{
    initData();
}

template<int N>
Ngons<N>::Data::Data(const size_t numCorners, const size_t numCoords, const std::string &name, const Meta &meta)
: Base::Data(numCoords, N == 3 ? Object::TRIANGLES : Object::QUADS, name, meta)
{
    CHECK_OVERFLOW(numCorners);
    initData();
    cl.construct(numCorners);
    ghost.construct(0);
}


template<int N>
typename Ngons<N>::Data *Ngons<N>::Data::create(const size_t numCorners, const size_t numCoords, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *t = shm<Data>::construct(name)(numCorners, numCoords, name, meta);
    publish(t);

    return t;
}

template<int N>
Index Ngons<N>::getNumElements()
{
    return getNumCorners() > 0 ? getNumCorners() / N : getNumCoords() / N;
}

template<int N>
Index Ngons<N>::getNumElements() const
{
    return getNumCorners() > 0 ? getNumCorners() / N : getNumCoords() / N;
}

template<int N>
Index Ngons<N>::getNumCorners()
{
    return d()->cl->size();
}

template<int N>
Index Ngons<N>::getNumCorners() const
{
    return m_numCorners;
}

template<int N>
void Ngons<N>::resetCorners()
{
    d()->cl = ShmVector<Index>();
    d()->cl.construct();

    d()->ghost = ShmVector<Byte>();
    d()->ghost.construct();

    refreshImpl();
}

template<int N>
Ngons<N>::Ngons(Data *data): Ngons::Base(data)
{
    refreshImpl();
}

#if 0
V_OBJECT_CREATE_NAMED(Vec<T,Dim>)
#else
template<int N>
Ngons<N>::Data::Data(Object::Type id, const std::string &name, const Meta &m): Ngons<N>::Base::Data(id, name, m)
{
    initData();
}

template<int N>
typename Ngons<N>::Data *Ngons<N>::Data::createNamed(Object::Type id, const std::string &name)
{
    Data *t = shm<Data>::construct(name)(id, name);
    publish(t);
    return t;
}
#endif
//V_OBJECT_CTOR(Triangles)

template<int N>
Object::Type Ngons<N>::type()
{
    return N == 3 ? Object::TRIANGLES : Object::QUADS;
}

template class Ngons<3>;
template class Ngons<4>;


#if 0
//V_OBJECT_TYPE(Triangles, Object::TRIANGLES)

//V_OBJECT_TYPE(Quads, Object::QUADS)
V_OBJECT_CTOR(Quads)
#endif

V_OBJECT_INST(Ngons<3>)
V_OBJECT_INST(Ngons<4>)

} // namespace vistle
