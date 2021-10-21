#ifndef VEC_TEMPLATE_H
#define VEC_TEMPLATE_H

#include "scalars.h"
#include "structuredgridbase.h"

#include <limits>
#include <type_traits>

#include <boost/mpl/size.hpp>

namespace vistle {

template<class T, int Dim>
Vec<T, Dim>::Vec(): Vec::Base()
{
    refreshImpl();
}

template<class T, int Dim>
Vec<T, Dim>::Vec(Data *data): Vec::Base(data)
{
    refreshImpl();
}

template<class T, int Dim>
Vec<T, Dim>::Vec(const Index size, const Meta &meta): Vec::Base(Data::create(size, meta))
{
    refreshImpl();
}

template<class T, int Dim>
void Vec<T, Dim>::resetArrays()
{
    for (int c = 0; c < Dim; ++c) {
        d()->x[c] = ShmVector<T>();
        d()->x[c].construct();
    }
    refresh();
}

template<class T, int Dim>
void Vec<T, Dim>::setSize(const Index size)
{
    for (int c = 0; c < Dim; ++c) {
        if (d()->x[c].valid()) {
            d()->x[c]->resize(size);
        } else {
            d()->x[c].construct(size);
        }
    }
    refresh();
}

template<class T, int Dim>
bool Vec<T, Dim>::copyEntry(Index to, DataBase::const_ptr src, Index from)
{
    auto s = Vec<T, Dim>::as(src);
    if (!s)
        return false;

    for (int c = 0; c < Dim; ++c) {
        x(c)[to] = s->x(c)[from];
    }

    return true;
}

template<class T, int Dim>
void Vec<T, Dim>::setValue(Index idx, int component, const double &value)
{
    assert(component >= 0);
    assert(component < Dim);
    x(component)[idx] = value;
}

template<class T, int Dim>
double Vec<T, Dim>::value(Index idx, int component) const
{
    assert(component >= 0);
    assert(component < Dim);
    return static_cast<double>(x(component)[idx]);
}

template<class T, int Dim>
void Vec<T, Dim>::applyDimensionHint(Object::const_ptr grid)
{
    if (auto str = StructuredGridBase::as(grid)) {
        auto m = guessMapping(shared_from_this());
        for (int c = 0; c < Dim; ++c) {
            if (m == DataBase::Vertex) {
                d()->x[c]->setDimensionHint(str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2));
            } else if (m == DataBase::Element) {
                d()->x[c]->setDimensionHint(str->getNumDivisions(0) - 1,
                                            std::max<Index>(1, str->getNumDivisions(1) - 1),
                                            std::max<Index>(1, str->getNumDivisions(2) - 1));
            }
        }
    } else {
        for (int c = 0; c < Dim; ++c)
            d()->x[c]->clearDimensionHint();
    }
}

template<class T, int Dim>
void Vec<T, Dim>::setExact(bool exact)
{
    d()->setExact(exact);
}

template<class T, int Dim>
void Vec<T, Dim>::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);

    for (int c = 0; c < Dim; ++c) {
        m_x[c] = (d && d->x[c].valid()) ? d->x[c]->data() : nullptr;
    }
    for (int c = Dim; c < MaxDim; ++c) {
        m_x[c] = nullptr;
    }
    m_size = (d && d->x[0]) ? d->x[0]->size() : 0;
}

template<class T, int Dim>
bool Vec<T, Dim>::isEmpty() const
{
    return getSize() == 0;
}

template<class T, int Dim>
bool Vec<T, Dim>::isEmpty()
{
    return getSize() == 0;
}

template<class T, int Dim>
bool Vec<T, Dim>::checkImpl() const
{
    size_t size = d()->x[0]->size();
    for (int c = 0; c < Dim; ++c) {
        V_CHECK(d()->x[c]->check());
        V_CHECK(d()->x[c]->size() == size);
        if (size > 0) {
            V_CHECK((d()->x[c])->at(0) * (d()->x[c])->at(0) >= 0)
            V_CHECK((d()->x[c])->at(size - 1) * (d()->x[c])->at(size - 1) >= 0)
        }
    }

    return true;
}

template<class T, int Dim>
void Vec<T, Dim>::updateInternals()
{
    if (!d()->boundsValid())
        d()->updateBounds();
}

template<class T, int Dim>
std::pair<typename Vec<T, Dim>::Vector, typename Vec<T, Dim>::Vector> Vec<T, Dim>::getMinMax() const
{
    if (d()->boundsValid()) {
        return std::make_pair(d()->min, d()->max);
    }

    T smax = std::numeric_limits<T>::max();
    T smin = std::numeric_limits<T>::lowest();
    Vector min, max;
    Index sz = getSize();
    for (int c = 0; c < Dim; ++c) {
        min[c] = smax;
        max[c] = smin;
        const T *d = &x(c)[0];
        for (Index i = 0; i < sz; ++i) {
            if (d[i] < min[c])
                min[c] = d[i];
            if (d[i] > max[c])
                max[c] = d[i];
        }
    }
    return std::make_pair(min, max);
}

template<class T, int Dim>
void Vec<T, Dim>::Data::initData()
{
    invalidateBounds();
}

template<class T, int Dim>
bool Vec<T, Dim>::Data::boundsValid() const
{
    for (int c = 0; c < Dim; ++c)
        if (min[c] > max[c])
            return false;
    return true;
}

template<class T, int Dim>
void Vec<T, Dim>::Data::invalidateBounds()
{
    for (int c = 0; c < Dim; ++c) {
        min[c] = std::numeric_limits<T>::max();
        max[c] = std::numeric_limits<T>::lowest();
    }
}

template<class T, int Dim>
void Vec<T, Dim>::Data::updateBounds()
{
    invalidateBounds();
    for (int c = 0; c < Dim; ++c) {
        Index sz = x[c]->size();
        const T *d = x[c]->data();
        for (Index i = 0; i < sz; ++i) {
            if (d[i] < min[c])
                min[c] = d[i];
            if (d[i] > max[c])
                max[c] = d[i];
        }
    }
}

template<class T, int Dim>
void Vec<T, Dim>::Data::setExact(bool exact)
{
    for (int c = 0; c < Dim; ++c)
        if (x[c])
            x[c]->setExact(exact);
}

template<class T, int Dim>
Vec<T, Dim>::Data::Data(const Index size, const std::string &name, const Meta &m)
: Vec<T, Dim>::Base::Data(Vec<T, Dim>::type(), name, m)
{
    initData();

    for (int c = 0; c < Dim; ++c)
        x[c].construct(size);
}

template<class T, int Dim>
Object::Type Vec<T, Dim>::type()
{
    static const size_t pos = boost::mpl::find<Scalars, T>::type::pos::value;
    static_assert(pos < boost::mpl::size<Scalars>::value, "Scalar type not found");
    return (Object::Type)(Object::VEC + (MaxDim + 1) * pos + Dim);
}

template<class T, int Dim>
Vec<T, Dim>::Data::Data(const Index size, Type id, const std::string &name, const Meta &m)
: Vec<T, Dim>::Base::Data(id, name, m)
{
    initData();

    for (int c = 0; c < Dim; ++c)
        x[c].construct(size);
}

template<class T, int Dim>
Vec<T, Dim>::Data::Data(const Data &o, const std::string &n, Type id)
: Vec<T, Dim>::Base::Data(o, n, id == Object::UNKNOWN ? o.type : id)
{
    initData();

    for (int c = 0; c < Dim; ++c)
        x[c] = o.x[c];
}

template<class T, int Dim>
typename Vec<T, Dim>::Data *Vec<T, Dim>::Data::create(Index size, const Meta &meta)
{
    std::string name = Shm::the().createObjectId();
    Data *t = shm<Data>::construct(name)(size, name, meta);
    publish(t);

    return t;
}

#if 0
V_OBJECT_CREATE_NAMED(Vec<T,Dim>)
#else
template<class T, int Dim>
Vec<T, Dim>::Data::Data(Object::Type id, const std::string &name, const Meta &m): Vec<T, Dim>::Base::Data(id, name, m)
{
    initData();
}

template<class T, int Dim>
typename Vec<T, Dim>::Data *Vec<T, Dim>::Data::createNamed(Object::Type id, const std::string &name, const Meta &meta)
{
    Data *t = shm<Data>::construct(name)(id, name, meta);
    publish(t);
    return t;
}
#endif

} // namespace vistle

#endif
