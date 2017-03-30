// Core/Parse.hpp

#ifndef _CORE_PARSE_HPP_INCLUDED_
#define _CORE_PARSE_HPP_INCLUDED_

#include <string>
#include <type_traits>

namespace Core
{
	namespace detail
	{
		template <typename T>
		inline T Parse(const char* str);

		///////////////////////////////////// BUILT-IN TYPES /////////////////////////////////////

		template <>
		inline char Parse(const char* str)
		{
			return (char)atoi(str);
		}

		template <>
		inline unsigned char Parse(const char* str)
		{
			return (unsigned char)atoi(str);
		}

		template <>
		inline short Parse(const char* str)
		{
			return (short)atoi(str);
		}

		template <>
		inline unsigned short Parse(const char* str)
		{
			return (unsigned short)atoi(str);
		}

		template <>
		inline int Parse(const char* str)
		{
			return atoi(str);
		}

		template <>
		inline unsigned Parse(const char* str)
		{
			return (unsigned)atoll(str);
		}

		template <>
		inline long long Parse(const char* str)
		{
			return atoll(str);
		}

		template <>
		inline unsigned long long Parse(const char* str)
		{
			char* pEnd;
			return strtoull(str, &pEnd, 10);
		}

		template <>
		inline float Parse(const char* str)
		{
			return (float)atof(str);
		}

		template <>
		inline double Parse(const char* str)
		{
			return atof(str);
		}

		template <>
		inline bool Parse(const char* str)
		{
			return (atoi(str) != 0);
		}

		///////////////////////////////////// STL TYPES /////////////////////////////////////

		template <>
		inline std::string Parse(const char* str)
		{
			return str;
		}

		////////////////////////////////// HELPER CLASSES //////////////////////////////////

		template <typename T>
		struct TypeParser
		{
			static T Parse(const char* str)
			{
				return Core::detail::Parse<T>(str);
			}
		};

		template <typename T>
		struct EnumParser
		{
			static T Parse(const char* str)
			{
				return static_cast<T>(Core::detail::Parse<std::underlying_type<T>::type>(str));
			}
		};

		///////////////////////////////////////////////////////////////////////////////////

		template <typename T>
		using Parser = typename std::conditional<std::is_enum<T>::value, EnumParser<T>, TypeParser<T>>::type;
	};

	template <typename T>
	inline T Parse(const char* str)
	{
		return detail::Parser<T>::Parse(str);
	}

	template <typename T>
	inline T Parse(const std::string& str)
	{
		return Parse<T>(str.c_str());
	}
}

#endif
