#ifndef INSITU_DATA_TYPE_H
#define INSITU_DATA_TYPE_H
#include "export.h"

#include <vistle/core/scalar.h>
#include <vistle/core/index.h>

namespace vistle {
namespace insitu {
enum class DataType
{
	INVALID
	, CHAR
	, INT
	, LONG
	, FLOAT
	, DOUBLE
	, INDEX //uint32_t
	, BYTE  //unsigned char
	, ARRAY
};
template<typename T>
constexpr DataType V_INSITUCOREEXPORT getDataType() {
	return DataType::INVALID;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<char>() {
	return DataType::CHAR;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<int>() {
	return DataType::INT;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<long>() {
	return DataType::LONG;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<float>() {
	return DataType::FLOAT;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<double>() {
	return DataType::DOUBLE;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<vistle::Index>() {
	return DataType::INDEX;
}
template<>
constexpr DataType V_INSITUCOREEXPORT getDataType<vistle::Byte>() {
	return DataType::BYTE;
}
}
}
#endif