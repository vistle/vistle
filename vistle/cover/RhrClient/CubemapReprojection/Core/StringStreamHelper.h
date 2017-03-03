// Core/StringStreamHelper.h

#ifndef _CORE_STRINGSTREAMHELPER_H_
#define _CORE_STRINGSTREAMHELPER_H_

#include <type_traits>
#include <cstdint>
#include <sstream>

template <typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
inline std::stringstream& operator<<(std::stringstream& ss, T value)
{
	// For char and unsigned char we want the string stream to write integers
	// and not to interpret them as charcters.
	ss << static_cast<std::conditional_t<
		sizeof(std::underlying_type_t<T>) < sizeof(std::int16_t),
		std::int16_t,
		std::underlying_type_t<T>>>(value);
	return ss;
}

#endif