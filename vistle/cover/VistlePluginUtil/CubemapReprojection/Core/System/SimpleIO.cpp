// Core/SimpleIO.cpp

#include <Core/System/SimpleIO.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Core
{
	namespace detail
	{
		inline void CheckOperationCorrectness(std::ios& fstream, const char* path, const char* operation)
		{
			if (fstream.fail())
			{
				std::stringstream ss;
				ss << "An error has occured while " << operation << ": " << path;
				throw std::runtime_error(ss.str().c_str());
			}
		}

		inline void CheckOpened(std::ios& fstream, const char* path)
		{
			CheckOperationCorrectness(fstream, path, "opening");
		}

		inline void CheckClosed(std::ios& fstream, const char* path)
		{
			CheckOperationCorrectness(fstream, path, "closing");
		}

		inline void CheckRead(std::ios& fstream, const char* path)
		{
			CheckOperationCorrectness(fstream, path, "reading");
		}

		inline void CheckWritten(std::ios& fstream, const char* path)
		{
			CheckOperationCorrectness(fstream, path, "writing");
		}
	}

	void ReadAllText(const char* path, std::string& text)
	{
		std::ifstream is(path, std::ios::in);
		detail::CheckOpened(is, path);

		// Note that this implementation avoids relying on file sizes, in which case line endings could mess up the reading.
		std::stringstream iss;
		iss << is.rdbuf();
		detail::CheckRead(is, path);
		text = iss.str();

		is.close();
		detail::CheckClosed(is, path);
	}

	void ReadAllText(const std::string& path, std::string& text)
	{
		ReadAllText(path.c_str(), text);
	}

	std::string ReadAllText(const char* path)
	{
		std::string text;
		ReadAllText(path, text);
		return text;
	}

	std::string ReadAllText(const std::string& path)
	{
		std::string text;
		ReadAllText(path, text);
		return text;
	}

	void ReadAllBytes(const char* path, Core::ByteVector& bytes)
	{
		std::ifstream is(path, std::ios::in | std::ios::binary | std::ios::ate);
		detail::CheckOpened(is, path);

		auto fileSize = is.tellg();
		is.seekg(0, std::ios::beg);
		detail::CheckOperationCorrectness(is, path, "getting size");
		bytes.Resize(fileSize);

		is.read(reinterpret_cast<char*>(bytes.GetArray()), fileSize);
		detail::CheckRead(is, path);

		is.close();
		detail::CheckClosed(is, path);
	}

	void ReadAllBytes(const std::string& path, Core::ByteVector& bytes)
	{
		ReadAllBytes(path.c_str(), bytes);
	}

	Core::ByteVector ReadAllBytes(const char* path)
	{
		Core::ByteVector bytes;
		ReadAllBytes(path, bytes);
		return bytes;
	}

	Core::ByteVector ReadAllBytes(const std::string& path)
	{
		Core::ByteVector bytes;
		ReadAllBytes(path, bytes);
		return bytes;
	}

	void WriteAllText(const char* path, const char* text)
	{
		std::ofstream os(path, std::ios::out);
		detail::CheckOpened(os, path);

		os.write(text, strlen(text));
		detail::CheckWritten(os, path);

		os.close();
		detail::CheckClosed(os, path);
	}

	void WriteAllText(const std::string& path, const char* text)
	{
		WriteAllText(path.c_str(), text);
	}

	void WriteAllText(const char* path, const std::string& text)
	{
		WriteAllText(path, text.c_str());
	}

	void WriteAllText(const std::string& path, const std::string& text)
	{
		WriteAllText(path.c_str(), text.c_str());
	}

	void WriteAllBytes(const char* path, const void* bytes, size_t size)
	{
		std::ofstream os(path, std::ios::out | std::ios::binary);
		detail::CheckOpened(os, path);

		os.write(reinterpret_cast<const char*>(bytes), size);
		detail::CheckWritten(os, path);

		os.close();
		detail::CheckClosed(os, path);
	}

	void WriteAllBytes(const std::string& path, const void* bytes, size_t size)
	{
		WriteAllBytes(path.c_str(), bytes, size);
	}

	void WriteAllBytes(const char* path, const Core::ByteVector& bytes)
	{
		WriteAllBytes(path, bytes.GetArray(), bytes.GetSize());
	}

	void WriteAllBytes(const std::string& path, const Core::ByteVector& bytes)
	{
		WriteAllBytes(path, bytes.GetArray(), bytes.GetSize());
	}
}