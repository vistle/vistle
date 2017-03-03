// Core/Filesystem.h

#ifndef _CORE_FILESYSTEM_H_INCLUDED_
#define _CORE_FILESYSTEM_H_INCLUDED_

#include <Core/Platform.h>

#ifdef IS_WINDOWS
#include <filesystem>
#else
#include <Core/String.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <cassert>
#include <cstring>
#endif

namespace Core
{
#ifndef IS_WINDOWS

	namespace detail
	{
		enum class PathTokenType : unsigned char
		{
			RootName,		// Optional (e.g. "//server")
			RootDirectory,	// (e.g. "/")
			FileName,		// Name of a directory, regular file or links (e.g. "etc" or "log.txt").
			Current,		// "."
			Parent			// ".."
		};

		struct PathToken
		{
			std::string Name;
			PathTokenType Type;
		};

		struct Path
		{
			std::vector<PathToken> Tokens;
		};

		inline void _AddToken(Path& path, PathTokenType type)
		{
			path.Tokens.push_back({ "", type });
		}

		inline void _AddToken(Path& path, const char* cPath, int start, int end, PathTokenType type)
		{
			auto size = end - start;
			auto buffer = new char[size + 1];
			strncpy(buffer, cPath + start, size);
			path.Tokens.push_back({ buffer, type });
		}

		inline Path ParsePath(const std::string& path)
		{
			int length = (int)path.length();
			char* cPath = new char[length + 1];
			strcpy(cPath, Core::Trim(path).c_str());
			for (auto pc = cPath; *pc; ++pc) if (*pc == '\\') *pc = '/';
			Path pPath;
			if (length == 0) return pPath;
			auto& tokens = pPath.Tokens;
			int index = 0;
			int startIndex = 0;
			if (cPath[0] == '/')
			{
				if (length > 1 && cPath[1] == '/')
				{
					for (index = 2; index < length && cPath[index] != '/'; ++index);
					_AddToken(pPath, cPath, 0, index, PathTokenType::RootName);
				}
				else { _AddToken(pPath, PathTokenType::RootDirectory); index = 1; }
			}
			while (index < length)
			{
				bool processed = false;
				switch (cPath[index])
				{
				case '/':
					if (++index == length) _AddToken(pPath, PathTokenType::Current);
					processed = true;
					break;
				case '.':
					if (++index == length) { _AddToken(pPath, PathTokenType::Current); processed = true; }
					else if (cPath[index] == '.')
					{
						_AddToken(pPath, PathTokenType::Parent); ++index; processed = true;
						assert(index == length || cPath[index] == '/');
					}
					else if (cPath[index] == '/')
					{
						_AddToken(pPath, PathTokenType::Current); processed = true;
					}
					else startIndex = index - 1;
					break;
				default: startIndex = index; break;
				}
				if (!processed)
				{
					for (; index < length && cPath[index] != '/'; ++index);
					_AddToken(pPath, cPath, startIndex, index, PathTokenType::FileName);
				}
			}
			delete[] cPath;
			return pPath;
		}

		inline std::string _ToString(const PathToken& token, bool hasEndingSlash)
		{
			std::string str;
			switch (token.Type)
			{
			case PathTokenType::RootName: str += token.Name; break;
			case PathTokenType::RootDirectory: str += "/"; hasEndingSlash = false; break;
			case PathTokenType::FileName: str += token.Name; break;
			case PathTokenType::Current: str += "."; break;
			case PathTokenType::Parent: str += ".."; break;
			}
			if (hasEndingSlash) str += "/";
			return str;
		}

		inline std::string ToString(const PathToken& token)
		{
			return _ToString(token, false);
		}

		inline std::string ToString(const Path& path)
		{
			std::string str;
			for (size_t i = 0; i < path.Tokens.size(); i++)
			{
				str += _ToString(path.Tokens[i], i < path.Tokens.size() - 1);
			}
			return str;
		}

		inline void ReducePath(Path& path)
		{
			auto& tokens = path.Tokens;
			for (size_t i = 0; i < tokens.size();)
			{
				if (tokens[i].Type == PathTokenType::Current && i < tokens.size() - 1)
					tokens.erase(tokens.begin() + i);
				else ++i;
			}
			for (size_t i = 1; i < tokens.size();)
			{
				if (tokens[i].Type == PathTokenType::Parent && tokens[i - 1].Type == PathTokenType::FileName)
				{
					--i;
					tokens.erase(tokens.begin() + i, tokens.begin() + (i + 2));
					if (i == tokens.size()) tokens.push_back({ "", PathTokenType::Current });
				}
				else ++i;
			}
		}

		inline Path GetParentPath(const Path& path)
		{
			auto pathCopy = path;
			ReducePath(pathCopy);
			auto& tokens = pathCopy.Tokens;
			if (tokens.size() == 0) return{};
			return{ std::vector<PathToken>(tokens.begin(), tokens.end() - 1) };
		}

		inline PathToken GetFileName(const Path& path)
		{
			return path.Tokens.back();
		}

		inline bool IsAbsolutePath(const Path& path)
		{
			auto& tokens = path.Tokens;
			if (tokens.size() == 0) return false;
			auto type = tokens[0].Type;
			return (type == PathTokenType::RootName || type == PathTokenType::RootDirectory);
		}

		inline bool IsRelativePath(const Path& path)
		{
			return !IsAbsolutePath(path);
		}
	}

#endif

	inline std::string GetParentPath(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::path(path).parent_path().string();
#else
		return detail::ToString(detail::GetParentPath(detail::ParsePath(path)));
#endif
	}

	inline std::string GetFileName(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::path(path).filename().string();
#else
		return detail::ToString(detail::GetFileName(detail::ParsePath(path)));
#endif
	}

	inline bool IsRelativePath(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::path(path).is_relative();
#else
		return detail::IsRelativePath(detail::ParsePath(path));
#endif
	}

	inline long long GetLastWriteTime(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::last_write_time(path).time_since_epoch().count();
#else
		struct stat path_stat;
		stat(path.c_str(), &path_stat);
		return static_cast<long long>(path_stat.st_mtime);
#endif
	}

	inline bool FileExists(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::exists(path);
#else
		return (access(path.c_str(), F_OK) == 0);
#endif
	}

	// Returns true if the path is an EXISTING directory. For paths of non-existent files
	// it can't be decided whether it's a regular file or a directory.
	inline bool IsDirectory(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::is_directory(path);
#else
		struct stat path_stat;
		stat(path.c_str(), &path_stat);
		return S_ISDIR(path_stat.st_mode);
#endif
	}

	// Returns true if the path is an EXISTING regular file. For paths of non-existent files
	// it can't be decided whether it's a regular file or a directory.
	inline bool IsRegularFile(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::is_regular_file(path);
#else
		struct stat path_stat;
		stat(path.c_str(), &path_stat);
		return S_ISREG(path_stat.st_mode);
#endif
	}

	inline std::string GetFileExtension(const std::string& path)
	{
#ifdef IS_WINDOWS
		return std::experimental::filesystem::path(path).extension().string();
#else
		auto pPath = detail::ParsePath(path);
		auto& lastToken = pPath.Tokens.back();
		if (lastToken.Type != detail::PathTokenType::FileName) return std::string();
		auto lastPointIndex = lastToken.Name.find_last_of('.');
		if (lastPointIndex == std::string::npos) return std::string();
		return lastToken.Name.substr(lastPointIndex);
#endif
	}

	inline void CreateDirectory_(const std::string& path) // Ignoring name collision with Windows macro.
	{
#ifdef IS_WINDOWS
		std::experimental::filesystem::create_directory(path);
#else
		mkdir(path.c_str(), S_IRWXU);
#endif
	}

	inline void RemoveFile(const std::string& path)
	{
#ifdef IS_WINDOWS
		std::experimental::filesystem::remove(path);
#else
		remove(path.c_str());
#endif
	}

	inline void CreateDirectoryPath(const std::string& path)
	{
		if (!FileExists(path))
		{
			CreateDirectoryPath(GetParentPath(path));
			CreateDirectory_(path);
		}
	}

	// Creates all directories to the given path.
	// If the path is a directory, than this directory itself also gets created.
	// For non-existing directories the path MUST end with a terminal slash ('/' or '\\').
	inline void PreparePath(const std::string& path)
	{
		CreateDirectoryPath(GetParentPath(path));
	}
}

#endif