// Core/String.hpp

#ifndef _CORE_STRING_HPP_
#define _CORE_STRING_HPP_

#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <locale>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cstdarg>
#include <limits>
#include <cassert>

#include <Core/Platform.h>

namespace Core
{
	/////////////////////////////////////// CASE ///////////////////////////////////////

	inline std::string ToLower(const std::string& str)
	{
		auto result = str;
		std::transform(str.begin(), str.end(), result.begin(), ::tolower);
		return result;
	}

	inline std::string ToUpper(const std::string& str)
	{
		auto result = str;
		std::transform(str.begin(), str.end(), result.begin(), ::toupper);
		return result;
	}

	/////////////////////////////////////// PREDICTORS ///////////////////////////////////////

	class IsCharacter
	{
		char m_C;

	public:

		IsCharacter(char c) : m_C(c) {}

		bool operator()(char ch) const
		{
			return (ch == m_C);
		}
	};

	class IsAnyOf
	{
		std::vector<char> m_Chars;

	public:

		IsAnyOf(const char* chars)
		{
			size_t length = strlen(chars);
			m_Chars.reserve(length);
			for (size_t i = 0; i < length; i++)
			{
				m_Chars.push_back(chars[i]);
			}
		}

		bool operator()(char ch) const
		{
			auto end = m_Chars.end();
			return (std::find(m_Chars.begin(), end, ch) != end);
		}
	};

	class IsWhitespace
	{
	public:

		bool operator()(char ch) const
		{
			return (std::isspace(ch) != 0);
		}
	};

	class IsNewLine
	{
	public:

		bool operator()(char ch) const
		{
			return (ch == '\n' || ch == '\r');
		}
	};

	/////////////////////////////////////// TRIM ///////////////////////////////////////

	namespace detail
	{
		template <typename PredicateType>
		inline size_t GetLeftTrimIndex(const std::string& str, const PredicateType& predicate)
		{
			size_t length = str.length();
			size_t i = 0;
			for (; i < length; i++)
			{
				if (!predicate(str[i]))
				{
					break;
				}
			}
			return i;
		}

		template <typename PredicateType>
		inline size_t GetRightTrimIndex(const std::string& str, const PredicateType& predicate)
		{
			size_t length = str.length();
			if (length == 0)
			{
				return 0;
			}
			size_t i = str.length() - 1;
			for (; i > 0; i--)
			{
				if (!predicate(str[i]))
				{
					break;
				}
			}
			if (i == 0 && predicate(str[0]))
			{
				return 0;
			}
			return (i + 1);
		}

		inline void CopyToBegining(std::string& str, size_t startIndex, size_t endIndex)
		{
			if (endIndex < startIndex)
			{
				str.resize(0);
			}
			else
			{
				size_t length = endIndex - startIndex;
				for (size_t targetIndex = 0; targetIndex < length; targetIndex++, startIndex++)
				{
					str[targetIndex] = str[startIndex];
				}
				str.resize(length);
			}
		}
	}

	template <typename PredicateType>
	inline std::string TrimLeft(const std::string& str, const PredicateType& predicate)
	{
		return str.substr(detail::GetLeftTrimIndex(str, predicate));
	}

	template <typename PredicateType>
	inline std::string TrimRight(const std::string& str, const PredicateType& predicate)
	{
		return str.substr(0, detail::GetRightTrimIndex(str, predicate));
	}

	template <typename PredicateType>
	inline std::string Trim(const std::string& str, const PredicateType& predicate)
	{
		size_t leftIndex = detail::GetLeftTrimIndex(str, predicate);
		size_t rightIndex = detail::GetRightTrimIndex(str, predicate);
		return str.substr(leftIndex, rightIndex - leftIndex);
	}

	template <typename PredicateType>
	inline std::string&& TrimLeft(std::string&& str, const PredicateType& predicate)
	{
		detail::CopyToBegining(str, detail::GetLeftTrimIndex(str, predicate), str.length());
		return std::move(str);
	}

	template <typename PredicateType>
	inline std::string&& TrimRight(std::string&& str, const PredicateType& predicate)
	{
		detail::CopyToBegining(str, 0, detail::GetRightTrimIndex(str, predicate));
		return std::move(str);
	}

	template <typename PredicateType>
	inline std::string&& Trim(std::string&& str, const PredicateType& predicate)
	{
		detail::CopyToBegining(str, detail::GetLeftTrimIndex(str, predicate), detail::GetRightTrimIndex(str, predicate));
		return std::move(str);
	}

	inline std::string TrimLeft(const std::string& str)
	{
		return TrimLeft(str, IsWhitespace());
	}

	inline std::string TrimRight(const std::string& str)
	{
		return TrimRight(str, IsWhitespace());
	}

	inline std::string Trim(const std::string& str)
	{
		return Trim(str, IsWhitespace());
	}

	inline std::string&& TrimLeft(std::string&& str)
	{
		return TrimLeft(std::move(str), IsWhitespace());
	}

	inline std::string&& TrimRight(std::string&& str)
	{
		return TrimRight(std::move(str), IsWhitespace());
	}

	inline std::string&& Trim(std::string&& str)
	{
		return Trim(std::move(str), IsWhitespace());
	}

	template <typename PredicateType>
	inline void TrimInPlace(std::string& str, const PredicateType& predicate)
	{
		Trim(std::move(str), predicate);
	}

	template <typename PredicateType>
	inline void TrimLeftInPlace(std::string& str, const PredicateType& predicate)
	{
		TrimLeft(std::move(str), predicate);
	}

	template <typename PredicateType>
	inline void TrimRightInPlace(std::string& str, const PredicateType& predicate)
	{
		TrimRight(std::move(str), predicate);
	}

	inline void TrimInPlace(std::string& str)
	{
		Trim(std::move(str));
	}

	inline void TrimLeftInPlace(std::string& str)
	{
		TrimLeft(std::move(str));
	}

	inline void TrimRightInPlace(std::string& str)
	{
		TrimRight(std::move(str));
	}

	/////////////////////////////////////// IS STARTING WITH ///////////////////////////////////////

	namespace detail
	{
		inline bool IsStartingWith(const std::string& str, const std::string& test, size_t startIndex)
		{
			size_t testLength = test.length();
			for (size_t i = 0; i < testLength; i++)
			{
				if (str[startIndex + i] != test[i])
				{
					return false;
				}
			}
			return true;
		}
	}

	inline bool IsStartingWith(const std::string& str, const std::string& test)
	{
		if (test.length() > str.length())
		{
			return false;
		}
		return detail::IsStartingWith(str, test, 0);
	}

	/////////////////////////////////////// IS CONTAINING ///////////////////////////////////////

	inline bool IsContaining(const std::string& str, const std::string& test)
	{
		size_t length = str.length();
		size_t testLength = test.length();
		if (length < testLength)
		{
			return false;
		}
		size_t checkLength = length - testLength;
		for (size_t i = 0; i <= checkLength; i++)
		{
			bool isMatching = true;
			for (size_t j = 0; j < testLength; j++)
			{
				if (str[i + j] != test[j])
				{
					isMatching = false;
					break;
				}
			}
			if (isMatching)
			{
				return true;
			}
		}
		return false;
	}

	/////////////////////////////////////// REPLACE ///////////////////////////////////////

	namespace detail
	{
		inline void AppendToCharVector(std::vector<char>& charVector, const std::string& str)
		{
			size_t length = str.length();
			for (size_t i = 0; i < length; i++)
			{
				charVector.push_back(str[i]);
			}
		}

		inline void AppendToCharVector(std::vector<char>& charVector, const std::string& str, size_t startIndex)
		{
			size_t length = str.length();
			for (size_t i = startIndex; i < length; i++)
			{
				charVector.push_back(str[i]);
			}
		}

		inline void AppendToCharVector(std::vector<char>& charVector, const std::string& str, size_t startIndex, size_t endIndex)
		{
			for (size_t i = startIndex; i < endIndex; i++)
			{
				charVector.push_back(str[i]);
			}
		}

		inline std::string CreateFromCharVector(std::vector<char>& charVector)
		{
			size_t length = charVector.size();
			std::string result;
			result.reserve(length);
			for (size_t i = 0; i < length; i++)
			{
				result.append(1, charVector[i]);
			}
			return result;
		}
	}

	inline std::string Replace(const std::string& str, const std::string& test, const std::string& replacement)
	{
		size_t length = str.length();
		size_t testLength = test.length();

		if (testLength == 0)
		{
			return str;
		}

		std::vector<char> result;
		result.reserve(length);

		size_t i = 0;
		for (; i + testLength <= length; i++)
		{
			if (detail::IsStartingWith(str, test, i))
			{
				detail::AppendToCharVector(result, replacement);
				i += (testLength - 1);
			}
			else
			{
				result.push_back(str[i]);
			}
		}
		for (; i < length; i++)
		{
			result.push_back(str[i]);
		}
		return detail::CreateFromCharVector(result);
	}

	inline std::string ReplaceFirst(const std::string& str, const std::string& test, const std::string& replacement)
	{
		size_t length = str.length();
		size_t testLength = test.length();

		if (testLength == 0)
		{
			return str;
		}

		std::vector<char> result;
		result.reserve(length);

		size_t i = 0;
		for (; i + testLength <= length; i++)
		{
			if (detail::IsStartingWith(str, test, i))
			{
				detail::AppendToCharVector(result, replacement);
				i += testLength;
				break;
			}
			else
			{
				result.push_back(str[i]);
			}
		}
		for (; i < length; i++)
		{
			result.push_back(str[i]);
		}
		return detail::CreateFromCharVector(result);
	}

	inline std::string ReplaceLast(const std::string& str, const std::string& test, const std::string& replacement)
	{
		size_t length = str.length();
		size_t testLength = test.length();

		if (testLength == 0 || testLength > length)
		{
			return str;
		}

		size_t lastReplacePlace = length;

		for (size_t i = length - testLength; i > 0; i--)
		{
			if (detail::IsStartingWith(str, test, i))
			{
				lastReplacePlace = i;
				break;
			}
		}
		if (lastReplacePlace == length && detail::IsStartingWith(str, test, 0))
		{
			lastReplacePlace = 0;
		}

		if (lastReplacePlace < length)
		{
			std::vector<char> result;
			result.reserve(length - testLength + replacement.length());
			detail::AppendToCharVector(result, str, 0, lastReplacePlace);
			detail::AppendToCharVector(result, replacement);
			detail::AppendToCharVector(result, str, lastReplacePlace + testLength);
			return detail::CreateFromCharVector(result);
		}

		return str;
	}

	/////////////////////////////////////// SPLIT ///////////////////////////////////////
	
	template <typename PredicateType>
	inline void Split(const char* str, size_t length, const PredicateType& predicate,
		bool isIgnoringEmptyResults, std::vector<std::string>& result)
	{
		result.clear();
		size_t start = 0;
		for (size_t i = 0; i < length; i++)
		{
			if (predicate(str[i]))
			{
				if (i > start || !isIgnoringEmptyResults)
				{
					result.push_back(std::string(str + start, str + i));
				}
				start = i + 1;
			}
		}
		if (length > start || !isIgnoringEmptyResults)
		{
			result.push_back(std::string(str + start, str + length));
		}
	}

	template <typename PredicateType>
	inline std::vector<std::string> Split(const char* str, size_t length, const PredicateType& predicate,
		bool isIgnoringEmptyResults)
	{
		std::vector<std::string> result;
		Split(str, length, predicate, isIgnoringEmptyResults, result);
		return result;
	}

	template <typename PredicateType>
	inline std::vector<std::string> Split(const char* str, const PredicateType& predicate, bool isIgnoringEmptyResults)
	{
		return Split(str, strlen(str), predicate, isIgnoringEmptyResults);
	}

	template <typename PredicateType>
	inline void Split(const std::string& str, const PredicateType& predicate,
		bool isIgnoringEmptyResults, std::vector<std::string>& result)
	{
		Split(str.data(), str.length(), predicate, isIgnoringEmptyResults, result);
	}

	template <typename PredicateType>
	inline std::vector<std::string> Split(const std::string& str, const PredicateType& predicate, bool isIgnoringEmptyResults)
	{
		return Split(str.c_str(), str.length(), predicate, isIgnoringEmptyResults);
	}

	inline std::vector<std::string> Split(const std::string& str, bool isIgnoringEmptyResults)
	{
		return Split(str, IsWhitespace(), isIgnoringEmptyResults);
	}

	//////////////////////////////////////// HASH ////////////////////////////////////////

	enum class StringHashAlgorithm
	{
		DJB2, SDBM
	};

	// Implementing known string hash algorithms. Source: http://www.cse.yorku.ca/~oz/hash.html
	inline const unsigned long GetHash(const std::string& str, StringHashAlgorithm hashAlgorithm = StringHashAlgorithm::SDBM)
	{
		if (hashAlgorithm == StringHashAlgorithm::DJB2)
		{
			unsigned long hash = 5381;

			size_t strLength = str.length();
			for (size_t i = 0; i < strLength; i++)
			{
				int c = str[i];
				hash = ((hash << 5) + hash) + c;
			}
			return hash;
		}
		else if (hashAlgorithm == StringHashAlgorithm::SDBM)
		{
			unsigned long hash = 0;

			size_t strLength = str.length();
			for (size_t i = 0; i < strLength; i++)
			{
				int c = str[i];
				hash = c + (hash << 6) + (hash << 16) - hash;
			}
			return hash;
		}

		throw new std::runtime_error("Unknown string hash algorithm.");
	}

	/////////////////////////////////////// FORMAT ///////////////////////////////////////

	template <class ... T>
	inline std::string Format(const std::string& format)
	{
		auto placeHolder = "%x";

		if (format.find(placeHolder) != std::string::npos)
		{
			throw std::runtime_error("String format error.");
		}
		return format;
	}

	template <class Head, class ... Tail>
	inline std::string Format(const std::string& format, const Head& head, const Tail& ... tail)
	{
		std::stringstream ss;

		auto placeHolder = "%x";
		unsigned placeHolderSize = 2;

		size_t firstPlaceholderPos = format.find(placeHolder);

		if (firstPlaceholderPos == std::string::npos)
		{
			throw std::runtime_error("String format error.");
		}
		else
		{
			std::string front(format, 0, firstPlaceholderPos);
			std::string back(format, firstPlaceholderPos + placeHolderSize);

			ss << front << head << Format(back, tail ...);
		}

		return ss.str();
	}

	/////////////////////////////////////// WSTRING ///////////////////////////////////////

	inline std::wstring ToWString(const std::string& str)
	{
		return std::wstring(str.begin(), str.end());
	}

	inline std::string ToString(const std::wstring& wStr)
	{
		return std::string(wStr.begin(), wStr.end());
	}

	/////////////////////////////////////// MISC ///////////////////////////////////////
	
	inline void Copy(const char* source, char* target)
	{
#ifdef IS_WINDOWS
		strcpy_s(target, strlen(source) + 1, source); // Assuming that the target buffer is always big enough.
#else
		strcpy(target, source);
#endif
	}

	inline void Copy(const char* source, char** pTarget)
	{
		*pTarget = new char[strlen(source) + 1];
		Copy(source, *pTarget);
	}
}

#endif