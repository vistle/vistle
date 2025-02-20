#ifndef VISLTE_LIBSIM_ARRAY_STRUCT_H
#define VISLTE_LIBSIM_ARRAY_STRUCT_H
#include "Exception.h"
#include "SmartHandle.h"
#include "SmartHandle.h"
#include "VisitDataTypesToVistle.h"

#include <vistle/insitu/core/transformArray.h>
#include <vistle/insitu/core/callFunctionWithVoidToTypeCast.h>
#include <vistle/insitu/libsim/libsimInterface/VariableData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

namespace vistle {
namespace insitu {
namespace libsim {
template<HandleType HT>
struct Array {
    void *data = nullptr;
    visit_smart_handle<HT> handle;
    int owner = VISIT_INVALID_HANDLE;
    int type = VISIT_INVALID_HANDLE;
    int dim = 1;
    size_t size = 0;

    template<typename T>
    struct Iter {
        Iter(const Array &array): m_array(array) {}
        const T *begin() const { return m_array.as<T>(); }
        const T *end() const { return begin() + m_array.size; }

    private:
        const Array &m_array;
    };
    template<typename T>
    const Iter<T> getIter() const
    {
        return Iter<T>(*this);
    }

    template<typename T>
    bool check() const
    {
        return dataTypeToVistle(type) == getDataType<T>();
    }

    template<typename T>
    T *as()
    {
        if (!check<T>()) {
            throw InsituException{} << "invalid Array access";
        }
        return static_cast<T *>(data);
    }
    template<typename T>
    const T *as() const
    {
        if (!check<T>()) {
            throw InsituException{} << "invalid Array access";
        }
        return static_cast<T *>(data);
    }
};
template<HandleType T>
inline Array<T> getVariableData(const visit_smart_handle<T> &handle)
{
    Array<T> a;
    a.handle = handle;
    int size;
    v2check(simv2_VariableData_getData, handle, a.owner, a.type, a.dim, size, a.data);
    assert(size >= 0);
    a.size = static_cast<size_t>(size);
    return a;
}

inline Array<HandleType::Coords> getVariableData(const visit_handle &handle)
{
    Array<HandleType::Coords> a;
    a.handle = handle;
    int size;
    v2check(simv2_VariableData_getData, handle, a.owner, a.type, a.dim, size, a.data);
    assert(size >= 0);
    a.size = static_cast<size_t>(size);
    return a;
}

template<typename T, HandleType HT>
void transformArray(const Array<HT> &source, T *dest)
{
    vistle::insitu::detail::callFunctionWithVoidToTypeCast<void, vistle::insitu::detail::ArrayTransformer>(
        source.data, dataTypeToVistle(source.type), source.size, dest);
}

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif
