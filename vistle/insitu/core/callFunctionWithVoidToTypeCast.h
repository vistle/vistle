#ifndef CALL_FUNCTION_WITH_VOID_TO_TYPE_CAST_H
#define CALL_FUNCTION_WITH_VOID_TO_TYPE_CAST_H
#include "dataType.h"
#include "exception.h"

namespace vistle {
namespace insitu {
namespace detail {
// cast the void* to dataType* and call Func(dataType*, size, Args2)
// Args1 must be equal to Args2 without pointer or reference
// the function object template operator() must look like this
/*
template<typename T, typename...args>
class FunctionObject {
    RetVal operator()(const T*, size_t, args...);
};
*/

template<typename RetVal, template<typename... Args1> class Func, typename... Args2>
RetVal callFunctionWithVoidToTypeCast(const void *source, DataType dataType, size_t size, Args2 &&...args)
{
    switch (dataType) {
    case DataType::CHAR: {
        const char *f = static_cast<const char *>(source);
        return Func<char, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    case DataType::INT: {
        const int *f = static_cast<const int *>(source);
        return Func<int, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    case DataType::FLOAT: {
        const float *f = static_cast<const float *>(source);
        return Func<float, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    case DataType::DOUBLE: {
        const double *f = static_cast<const double *>(source);
        return Func<double, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    case DataType::LONG: {
        const long *f = static_cast<const long *>(source);
        return Func<long, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    case DataType::INDEX: {
        const vistle::Index *f = static_cast<const vistle::Index *>(source);
        return Func<vistle::Index, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    case DataType::BYTE: {
        const vistle::Byte *f = static_cast<const vistle::Byte *>(source);
        return Func<vistle::Byte, typename std::decay<Args2>::type...>()(f, size, std::forward<Args2>(args)...);
    } break;
    default: {
        throw InsituException{} << "callFunctionWithVoidToTypeCast: non-numeric data types can not be converted";
    } break;
    }
}

} // namespace detail
} // namespace insitu
} // namespace vistle

#endif // !CALL_FUNCTION_WITH_VOID_TO_TYPE_CAST_H
