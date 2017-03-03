// Core/Enum.h

#ifndef _CORE_ENUM_H_
#define _CORE_ENUM_H_

#include <type_traits>

#define UseEnumAsFlagSet(enumName)														\
	inline enumName operator|(enumName a, enumName b)									\
	{																					\
	return static_cast<enumName>(static_cast<std::underlying_type_t<enumName>>(a)		\
	| static_cast<std::underlying_type_t<enumName>>(b));								\
	}																					\
	inline enumName& operator|=(enumName& a, enumName b)								\
	{																					\
	a = static_cast<enumName>(static_cast<std::underlying_type_t<enumName>>(a)			\
	| static_cast<std::underlying_type_t<enumName>>(b));								\
	return a;																			\
	}																					\
	inline bool HasFlag(enumName a, enumName b)											\
	{return ((static_cast<std::underlying_type_t<enumName>>(a)							\
	& static_cast<std::underlying_type_t<enumName>>(b)) != 0);							\
	}																					\
	inline bool IsSubset(enumName superSet, enumName subset)							\
	{return ((~static_cast<std::underlying_type_t<enumName>>(superSet)					\
	& static_cast<std::underlying_type_t<enumName>>(subset)) == 0);						\
	}

#endif