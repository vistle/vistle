// Core/Enum.h

#ifndef _CORE_ENUM_H_
#define _CORE_ENUM_H_

#include <type_traits>

#define UseEnumAsFlagSet(enumName)														\
	inline enumName operator|(enumName a, enumName b)									\
	{																					\
	return static_cast<enumName>(static_cast<std::underlying_type<enumName>::type>(a)		\
	| static_cast<std::underlying_type<enumName>::type>(b));								\
	}																					\
	inline enumName& operator|=(enumName& a, enumName b)								\
	{																					\
	a = static_cast<enumName>(static_cast<std::underlying_type<enumName>::type>(a)			\
	| static_cast<std::underlying_type<enumName>::type>(b));								\
	return a;																			\
	}																					\
	inline bool HasFlag(enumName a, enumName b)											\
        {return ((static_cast<std::underlying_type<enumName>::type>(a)							\
	& static_cast<std::underlying_type<enumName>::type>(b)) != 0);							\
	}																					\
	inline bool IsSubset(enumName superSet, enumName subset)							\
	{return ((~static_cast<std::underlying_type<enumName>::type>(superSet)					\
	& static_cast<std::underlying_type<enumName>::type>(subset)) == 0);						\
	}

#endif
