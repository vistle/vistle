// Core/SimpleIO.h

#ifndef _CORE_SIMPLEIO_H_INCLUDED_
#define _CORE_SIMPLEIO_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <string>

namespace Core
{
	enum class FileType : unsigned char
	{
		Binary, Text
	};

	enum class FileAccessType : unsigned char
	{
		Read, Write, Append
	};

	void ReadAllText(const char* path, std::string& text);
	void ReadAllText(const std::string& path, std::string& text);

	std::string ReadAllText(const char* path);
	std::string ReadAllText(const std::string& path);

	void ReadAllBytes(const char* path, Core::ByteVector& bytes);
	void ReadAllBytes(const std::string& path, Core::ByteVector& bytes);

	Core::ByteVector ReadAllBytes(const char* path);
	Core::ByteVector ReadAllBytes(const std::string& path);

	void WriteAllText(const char* path, const char* text);
	void WriteAllText(const std::string& path, const char* text);
	void WriteAllText(const char* path, const std::string& text);
	void WriteAllText(const std::string& path, const std::string& text);

	void WriteAllBytes(const char* path, const void* bytes, size_t size);
	void WriteAllBytes(const std::string& path, const void* bytes, size_t size);

	void WriteAllBytes(const char* path, const Core::ByteVector& bytes);
	void WriteAllBytes(const std::string& path, const Core::ByteVector& bytes);
}

#endif