#ifndef VEC_TEMPLATE_H
#define VEC_TEMPLATE_H

#include "scalars.h"
#include "structuredgridbase.h"
#include <vistle/util/exception.h>
#include "validate.h"

#include <limits>
#include <type_traits>

#include <boost/mpl/size.hpp>

namespace vistle {

template<class T, unsigned Dim>
Vec<T, Dim>::Vec(): Vec::Base()
{
    refreshImpl();
}

template<class T, unsigned Dim>
Vec<T, Dim>::Vec(Data *data): Vec::Base(data)
{
    refreshImpl();
}

template<class T, unsigned Dim>
Vec<T, Dim>::Vec(const size_t size, const Meta &meta): Vec::Base(Data::create(size, meta))
{
    CHECK_OVERFLOW(size);
    refreshImpl();
}

template<class T, unsigned Dim>
void Vec<T, Dim>::resetArrays()
{
    for (unsigned c = 0; c < Dim; ++c) {
        d()->x[c] = ShmVector<T>();
        d()->x[c].construct();
    }
    refresh();
}

template<class T, unsigned Dim>
void Vec<T, Dim>::setSize(const size_t size)
{
    CHECK_OVERFLOW(size);

    for (unsigned c = 0; c < Dim; ++c) {
        if (d()->x[c].valid()) {
            d()->x[c]->resize(size);
        } else {
            d()->x[c].construct(size);
        }
    }
    refresh();
}

template<class T, unsigned Dim>
bool Vec<T, Dim>::copyEntry(Index to, DataBase::const_ptr src, Index from)
{
    auto s = Vec<T, Dim>::as(src);
    if (!s)
        return false;

    for (unsigned c = 0; c < Dim; ++c) {
        x(c)[to] = s->x(c)[from];
    }

    return true;
}

template<class T, unsigned Dim>
void Vec<T, Dim>::setValue(Index idx, unsigned component, const double &value)
{
    assert(component >= 0);
    assert(component < Dim);
    x(component)[idx] = value;
}

template<class T, unsigned Dim>
double Vec<T, Dim>::value(Index idx, unsigned component) const
{
    assert(component >= 0);
    assert(component < Dim);
    return static_cast<double>(x(component)[idx]);
}

template<class T, unsigned Dim>
void Vec<T, Dim>::applyDimensionHint(Object::const_ptr grid)
{
    if (auto str = StructuredGridBase::as(grid)) {
        auto m = guessMapping(shared_from_this());
        for (unsigned c = 0; c < Dim; ++c) {
            if (m == DataBase::Vertex) {
                d()->x[c]->setDimensionHint(str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2));
            } else if (m == DataBase::Element) {
                d()->x[c]->setDimensionHint(str->getNumDivisions(0) - 1,
                                            std::max<Index>(1, str->getNumDivisions(1) - 1),
                                            std::max<Index>(1, str->getNumDivisions(2) - 1));
            }
        }
    } else {
        for (unsigned c = 0; c < Dim; ++c)
            d()->x[c]->clearDimensionHint();
    }
}

template<class T, unsigned Dim>
void Vec<T, Dim>::setExact(bool exact)
{
    d()->setExact(exact);
}

template<class T, unsigned Dim>
void Vec<T, Dim>::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);

    if (d) {
        for (unsigned c = 0; c < Dim; ++c) {
            m_x[c] = d->x[c];
        }
    } else {
        for (unsigned c = 0; c < Dim; ++c) {
            m_x[c] = nullptr;
        }
    }
    for (unsigned c = Dim; c < MaxDim; ++c) {
        m_x[c] = nullptr;
    }
    m_size = (d && d->x[0]) ? d->x[0]->size() : 0;
}

template<class T, unsigned Dim>
bool Vec<T, Dim>::isEmpty() const
{
    return getSize() == 0;
}

template<class T, unsigned Dim>
bool Vec<T, Dim>::isEmpty()
{
    return getSize() == 0;
}

template<class T, unsigned Dim>
bool Vec<T, Dim>::checkImpl(std::ostream &os, bool quick) const
{
    for (unsigned c = 0; c < Dim; ++c) {
        VALIDATE_INDEX(x(c).size());
    }
    size_t size = d()->x[0]->size();
    for (unsigned c = 0; c < Dim; ++c) {
        VALIDATE(d()->x[c]->check(os));
        VALIDATE(d()->x[c]->size() == size);
        if (size > 0) {
            VALIDATE((d()->x[c])->at(0) * (d()->x[c])->at(0) >= 0)
            VALIDATE((d()->x[c])->at(size - 1) * (d()->x[c])->at(size - 1) >= 0)
        }
    }

    if (quick)
        return true;

    bool valid = true;
    for (unsigned c = 0; c < Dim - 1; ++c) {
        const T *arr = x(c).data();
        for (Index i = 0; i < getSize(); ++i) {
            if (!(arr[i] * arr[i] >= 0)) {
                valid = false;
                os << "invalid data at " << i << ": " << arr[i] << std::endl;
            }
        }
    }
    return valid;
}


template<class T, unsigned Dim>
void Vec<T, Dim>::updateInternals()
{
    if (!d()->boundsValid()) {
        d()->updateBounds();
    }
    Base::updateInternals();
}

template<class T, unsigned Dim>
std::pair<typename Vec<T, Dim>::VecVector, typename Vec<T, Dim>::VecVector> Vec<T, Dim>::getMinMax() const
{
    VecVector min, max;
    if (d()->boundsValid()) {
        for (unsigned c = 0; c < Dim; ++c) {
            min[c] = d()->x[c]->min();
            max[c] = d()->x[c]->max();
        }
        return std::make_pair(min, max);
    }

    T smax = std::numeric_limits<T>::max();
    T smin = std::numeric_limits<T>::lowest();
    Index sz = getSize();
    for (unsigned c = 0; c < Dim; ++c) {
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

template<class T, unsigned Dim>
void Vec<T, Dim>::Data::initData()
{}

template<class T, unsigned Dim>
bool Vec<T, Dim>::Data::boundsValid() const
{
    for (unsigned c = 0; c < Dim; ++c) {
        if (x[c] && !x[c]->bounds_valid()) {
            return false;
        }
    }
    return true;
}

template<class T, unsigned Dim>
void Vec<T, Dim>::Data::invalidateBounds()
{
    for (unsigned c = 0; c < Dim; ++c) {
        if (x[c])
            x[c]->invalidate_bounds();
    }
}

template<class T, unsigned Dim>
void Vec<T, Dim>::Data::updateBounds()
{
    invalidateBounds();
    for (unsigned c = 0; c < Dim; ++c) {
        if (x[c])
            x[c]->update_bounds();
    }
}

template<class T, unsigned Dim>
void Vec<T, Dim>::Data::setExact(bool exact)
{
    for (unsigned c = 0; c < Dim; ++c)
        if (x[c])
            x[c]->setExact(exact);
}

template<class T, unsigned Dim>
Vec<T, Dim>::Data::Data(const size_t size, const std::string &name, const Meta &m)
: Vec<T, Dim>::Base::Data(Vec<T, Dim>::type(), name, m)
{
    CHECK_OVERFLOW(size);
    initData();

    for (unsigned c = 0; c < Dim; ++c)
        x[c].construct(size);
}

template<class T, unsigned Dim>
Object::Type Vec<T, Dim>::type()
{
    static const size_t pos = boost::mpl::find<Scalars, T>::type::pos::value;
    static_assert(pos < boost::mpl::size<Scalars>::value, "Scalar type not found");
    return (Object::Type)(Object::VEC + (MaxDim + 1) * pos + Dim);
}

template<class T, unsigned Dim>
Vec<T, Dim>::Data::Data(const size_t size, Type id, const std::string &name, const Meta &m)
: Vec<T, Dim>::Base::Data(id, name, m)
{
    CHECK_OVERFLOW(size);
    initData();

    for (unsigned c = 0; c < Dim; ++c)
        x[c].construct(size);
}

template<class T, unsigned Dim>
Vec<T, Dim>::Data::Data(const Data &o, const std::string &n, Type id)
: Vec<T, Dim>::Base::Data(o, n, id == Object::UNKNOWN ? o.type : id)
{
    initData();

    for (unsigned c = 0; c < Dim; ++c)
        x[c] = o.x[c];
}

template<class T, unsigned Dim>
typename Vec<T, Dim>::Data *Vec<T, Dim>::Data::create(size_t size, const Meta &meta)
{
    std::string name = Shm::the().createObjectId();
    Data *t = shm<Data>::construct(name)(size, name, meta);
    publish(t);

    return t;
}

#if 0
V_OBJECT_CREATE_NAMED(Vec<T,Dim>)
#else
template<class T, unsigned Dim>
Vec<T, Dim>::Data::Data(Object::Type id, const std::string &name, const Meta &m): Vec<T, Dim>::Base::Data(id, name, m)
{
    initData();
}

template<class T, unsigned Dim>
typename Vec<T, Dim>::Data *Vec<T, Dim>::Data::createNamed(Object::Type id, const std::string &name, const Meta &meta)
{
    Data *t = shm<Data>::construct(name)(id, name, meta);
    publish(t);
    return t;
}
#endif

template<class T, unsigned Dim>
void Vec<T, Dim>::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
    for (unsigned c = 0; c < Dim; ++c) {
        os << " ";
        switch (c) {
        case 0:
            os << "x";
            break;
        case 1:
            os << "y";
            break;
        case 2:
            os << "z";
            break;
        case 3:
            os << "w";
            break;
        default:
            os << "x" << c;
            break;
        }
        os << "(";
        d()->x[c]->print(os, verbose);
        os << ")";
    }
}

} // namespace vistle
#endif
