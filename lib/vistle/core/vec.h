#ifndef VEC_H
#define VEC_H
#include "index.h"
#include "dimensions.h"
#include "object.h"
#include "vector.h"
#include "database.h"
#include "shmvector.h"

namespace vistle {

template<typename T, int Dim = 1>
class Vec: public DataBase {
    V_OBJECT(Vec);

    static const int MaxDim = MaxDimension;
    static_assert(Dim > 0, "only positive Dim allowed");
    static_assert(Dim <= MaxDim, "Dim too large");

public:
    typedef DataBase Base;
    typedef typename shm<T>::array array;
    typedef T Scalar;
    typedef VistleVector<Scalar, Dim> VecVector;

    Vec(const Index size, const Meta &meta = Meta());

    Index getSize() override { return d()->x[0]->size(); }
    Index getSize() const override { return m_size; }

    int dimension() const override { return Dim; }

    bool copyEntry(Index to, DataBase::const_ptr src, Index from) override;
    void setValue(Index idx, int component, const double &value) override;
    double value(Index idx, int c = 0) const override;

    void resetArrays() override;
    void setSize(const Index size) override;
    void applyDimensionHint(Object::const_ptr grid) override;
    void setExact(bool exact) override;

    array &x(int c = 0) { return *d()->x[c]; }
    array &y()
    {
        assert(Dim > 1);
        return Dim > 1 ? *d()->x[1] : x();
    }
    array &z()
    {
        assert(Dim > 2);
        return Dim > 2 ? *d()->x[2] : x();
    }
    array &w()
    {
        assert(Dim > 3);
        return Dim > 3 ? *d()->x[3] : x();
    }

    const T *x(int c = 0) const { return m_x[c]; }
    const T *y() const
    {
        assert(Dim > 1);
        return Dim > 1 ? m_x[1] : x();
    }
    const T *z() const
    {
        assert(Dim > 2);
        return Dim > 2 ? m_x[2] : x();
    }
    const T *w() const
    {
        assert(Dim > 3);
        return Dim > 3 ? m_x[3] : x();
    }

    void updateInternals() override;
    std::pair<VecVector, VecVector> getMinMax() const;

private:
    mutable const T *m_x[MaxDim];
    mutable Index m_size;

public:
    struct Data: public Base::Data {
        VecVector min;
        VecVector max;
        ShmVector<T> x[Dim];
        // when used as Vec
        Data(const Index size = 0, const std::string &name = "", const Meta &meta = Meta());
        // when used as base of another data structure
        Data(const Index size, Type id, const std::string &name, const Meta &meta = Meta());
        Data(const Data &other, const std::string &name, Type id = UNKNOWN);
        Data(Object::Type id, const std::string &name, const Meta &meta);
        static Data *create(Index size = 0, const Meta &meta = Meta());
        static Data *createNamed(Object::Type id, const std::string &name, const Meta &meta = Meta());

    protected:
        void setExact(bool exact);

    private:
        friend class Vec;
        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar);
        void initData();
        void updateBounds();
        void invalidateBounds();
        bool boundsValid() const;
    };
};

#define V_VEC_COMMA ,
#define V_VEC_TEMPLATE(ValueType) \
    extern template class V_COREEXPORT Vec<ValueType, 1>; \
    V_OBJECT_DECL(Vec<ValueType V_VEC_COMMA 1>) \
    extern template class V_COREEXPORT Vec<ValueType, 2>; \
    V_OBJECT_DECL(Vec<ValueType V_VEC_COMMA 2>) \
    extern template class V_COREEXPORT Vec<ValueType, 3>; \
    V_OBJECT_DECL(Vec<ValueType V_VEC_COMMA 3>) \
    /* extern template class V_COREEXPORT Vec<ValueType,4>; \
    V_OBJECT_DECL(Vec<ValueType V_VEC_COMMA 4>) */
V_VEC_TEMPLATE(char)
V_VEC_TEMPLATE(signed char)
V_VEC_TEMPLATE(unsigned char)
V_VEC_TEMPLATE(int32_t)
V_VEC_TEMPLATE(uint32_t)
V_VEC_TEMPLATE(int64_t)
V_VEC_TEMPLATE(uint64_t)
V_VEC_TEMPLATE(float)
V_VEC_TEMPLATE(double)
#undef V_VEC_TEMPLATE
#undef V_VEC_COMMA

} // namespace vistle
#endif
