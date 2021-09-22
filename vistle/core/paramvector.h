#ifndef PARAMVECTOR_H
#define PARAMVECTOR_H

#include <vistle/util/sysdep.h>
#include <vistle/util/exception.h>
#include <ostream>
#include <vector>
#include <cassert>
#include <vistle/util/sysdep.h>
#include "scalar.h"
#include "dimensions.h"
#include "vector.h"
#include "export.h"

namespace vistle {

template<typename S>
class ParameterVector {
public:
    typedef S Scalar;
    static const int MaxDimension = vistle::MaxDimension;

    ParameterVector(const int dim, const S values[]);
    template<int Dim>
    ParameterVector(const ScalarVector<Dim> &v);
    ParameterVector(const S x, const S y, const S z, const S w);
    ParameterVector(const S x, const S y, const S z);
    ParameterVector(const S x, const S y);
    ParameterVector(const S x);
    ParameterVector();
    ParameterVector(const ParameterVector &other);

    ParameterVector &operator=(const ParameterVector &rhs);

    static ParameterVector min(int dim = MaxDimension);
    static ParameterVector max(int dim = MaxDimension);

    int dim;
    std::vector<S> v;
    std::vector<S> m_min, m_max;
    S &x, &y, &z, &w;

    S *data() { return v.data(); }
    const S *data() const { return v.data(); }

    S &operator[](int i) { return v[i]; }
    const S &operator[](int i) const { return v[i]; }

    operator S *() { return v.data(); }
    operator const S *() const { return v.data(); }

    operator S &()
    {
        assert(dim == 1);
        return v[0];
    }
    operator S() const
    {
        assert(dim == 1);
        return dim > 0 ? v[0] : S();
    }

    operator Vector1() const
    {
        assert(dim == 1);
        return dim >= 1 ? Vector1(v[0]) : Vector1(0.);
    }
    operator Vector2() const
    {
        assert(dim == 2);
        return dim >= 2 ? Vector2(v[0], v[1]) : Vector2(0., 0.);
    }
    operator Vector3() const
    {
        assert(dim == 3);
        return dim >= 3 ? Vector3(v[0], v[1], v[2]) : Vector3(0., 0., 0.);
    }
    operator Vector4() const
    {
        assert(dim == 4);
        return dim >= 4 ? Vector4(v[0], v[1], v[2], v[3]) : Vector4(0., 0., 0., 0.);
    }

    std::string str() const;

public:
    // for vector_indexing_suite
    typedef Scalar value_type;
    typedef size_t size_type;
    typedef size_t index_type;
    typedef ssize_t difference_type;
    bool empty() const { return dim == 0; }
    void clear() const { throw vistle::exception("not supported"); }
    size_t size() const { return dim; }
    typedef typename std::vector<S>::iterator iterator;
    typedef typename std::vector<S>::const_iterator const_iterator;
    void erase(iterator s)
    {
        throw except::not_implemented();
        v.erase(s);
    }
    void erase(iterator s, iterator e)
    {
        throw except::not_implemented();
        v.erase(s, e);
    }
    iterator begin() { return v.begin(); }
    const_iterator begin() const { return v.begin(); }
    iterator end() { return v.begin() + dim; }
    const_iterator end() const { return v.begin() + dim; }
    void insert(iterator pos, const S &value)
    {
        throw except::not_implemented();
        v.insert(pos, value);
    }
    template<class InputIt>
    void insert(iterator pos, InputIt first, InputIt last)
    {
        throw except::not_implemented();
        v.insert(pos, first, last);
    }
    ParameterVector(iterator begin, iterator end);
    void push_back(const S &value)
    {
        if (dim >= MaxDimension) {
            throw vistle::exception("out of range");
        }
        v[dim] = value;
        ++dim;
    }
    Scalar &back()
    {
        if (dim <= 0) {
            throw vistle::exception("out of range");
        }
        return v[0];
    }
    const Scalar &back() const
    {
        if (dim <= 0) {
            throw vistle::exception("out of range");
        }
        return v[0];
    }
    void pop_back()
    {
        if (dim <= 0) {
            throw vistle::exception("out of range");
        }
        --dim;
    }
    void reserve(size_t dim)
    {
        if (dim >= MaxDimension) {
            throw vistle::exception("out of range");
        }
    }
    void shrink_to_fit()
    {
        // nothing to do
    }
};

template<typename S>
bool operator==(const ParameterVector<S> &v1, const ParameterVector<S> &v2);
template<typename S>
bool operator!=(const ParameterVector<S> &v1, const ParameterVector<S> &v2);

template<typename S>
bool operator<(const ParameterVector<S> &v1, const ParameterVector<S> &v2);
template<typename S>
bool operator>(const ParameterVector<S> &v1, const ParameterVector<S> &v2);

template<typename S>
std::ostream &operator<<(std::ostream &out, const ParameterVector<S> &v);

#define V_DECLARE_PARAMVEC(S) \
    extern template class V_COREEXPORT ParameterVector<S>; \
    extern template V_COREEXPORT bool operator==(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    extern template V_COREEXPORT bool operator!=(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    extern template V_COREEXPORT bool operator<(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    extern template V_COREEXPORT bool operator>(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    extern template V_COREEXPORT std::ostream &operator<<(std::ostream &out, const ParameterVector<S> &v);

V_DECLARE_PARAMVEC(Float)
V_DECLARE_PARAMVEC(Integer)
typedef ParameterVector<Float> ParamVector;
typedef ParameterVector<Integer> IntParamVector;

} // namespace vistle

#endif // PARAMVECTOR_H
