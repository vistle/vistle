#ifndef VISTLE_INSITU_CORE_DATATYPE_H
#define VISTLE_INSITU_CORE_DATATYPE_H

#include "export.h"

#include <vistle/core/index.h>
#include <vistle/core/scalar.h>

namespace vistle {
namespace insitu {
enum class DataType {
    INVALID,
    CHAR,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    INDEX // uint32_t
    ,
    BYTE // unsigned char
    ,
    ARRAY
};
template<typename T>
constexpr DataType getDataType()
{
    return DataType::INVALID;
}
template<>
constexpr DataType getDataType<char>()
{
    return DataType::CHAR;
}
template<>
constexpr DataType getDataType<int>()
{
    return DataType::INT;
}
template<>
constexpr DataType getDataType<long>()
{
    return DataType::LONG;
}
template<>
constexpr DataType getDataType<float>()
{
    return DataType::FLOAT;
}
template<>
constexpr DataType getDataType<double>()
{
    return DataType::DOUBLE;
}
template<>
constexpr DataType getDataType<vistle::Index>()
{
    return DataType::INDEX;
}
template<>
constexpr DataType getDataType<vistle::Byte>()
{
    return DataType::BYTE;
}
} // namespace insitu
} // namespace vistle
#endif
