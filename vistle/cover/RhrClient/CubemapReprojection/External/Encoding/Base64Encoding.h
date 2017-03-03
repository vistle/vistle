// Encoding/Base64Encoding.h

#ifndef _ENCODING_BASE64ENCODING_H_
#define _ENCODING_BASE64ENCODING_H_

// Source: http://www.adp-gmbh.ch/cpp/common/base64.html

#include <string>
#include <cassert>

const std::string base64_chars = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

inline void StrechyInitialize(unsigned char** pData, size_t* pSize, size_t* pCapacity, size_t startCapacity)
{
	assert(startCapacity > 0);
	*pData = new unsigned char[startCapacity];
	*pSize = 0;
	*pCapacity = startCapacity;
}

inline void StrechyPushBack(unsigned char value, unsigned char** pData, size_t* pSize, size_t* pCapacity)
{
	size_t index = (*pSize)++;
	if (index == *pCapacity)
	{
		*pCapacity <<= 1;
		auto newData = new unsigned char[*pCapacity];
		memcpy(newData, *pData, index);
		delete[] * pData;
		*pData = newData;
	}
	(*pData)[index] = value;
}

inline std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;
}

inline void base64_decode(char const* encoded_string, size_t in_len, unsigned char** pData, size_t* pSize)
{
	size_t capacity;
	StrechyInitialize(pData, pSize, &capacity, 8);

	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				StrechyPushBack(char_array_3[i], pData, pSize, &capacity);
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) StrechyPushBack(char_array_3[j], pData, pSize, &capacity);
	}
}

#endif