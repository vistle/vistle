// EngineBuildingBlocks/PathHandler.cpp

#include <EngineBuildingBlocks/PathHandler.h>

#include <Core/Platform.h>
#include <Core/System/Filesystem.h>
#include <EngineBuildingBlocks/ErrorHandling.h>

#ifdef IS_WINDOWS
#include <Core/String.hpp>
#include "Windows.h"
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <queue>

namespace EngineBuildingBlocks
{
	std::string GetExecutablePath()
	{
		// Getting executable path.
#ifdef IS_WINDOWS

		wchar_t buffer[MAX_PATH];
		GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (FAILED(HRESULT_FROM_WIN32(GetLastError())))
		{
			EngineBuildingBlocks::RaiseException("Error getting executable path.");
		}
		return Core::ToString(buffer);

#else
		// Works in Linux.
		const unsigned maxPathSize = 1024;
		char id[256];
		char path[maxPathSize];
		sprintf(id, "/proc/%d/exe", getpid());
		ssize_t numChars = readlink(id, path, maxPathSize - 1);
		if (numChars == -1)
		{
			EngineBuildingBlocks::RaiseException("Error getting executable path.");
		}
		path[numChars] = '\0';
		return path;
#endif
	}

	std::string FindRootDirectory(const char* rootName, bool searchInChildrenDirs)
	{
		auto ending = std::string("/") + rootName + ".root";
		std::queue<std::string> queue;
		for (auto currentDirectory = Core::GetParentPath(GetExecutablePath()); !currentDirectory.empty();
			currentDirectory = Core::GetParentPath(currentDirectory))
		{
			if (Core::FileExists(currentDirectory + ending)) return currentDirectory;
			if (searchInChildrenDirs)
			{
				assert(queue.empty());
				queue.push(currentDirectory);

				while (!queue.empty())
				{
					auto dir = queue.front(); queue.pop();
					if (Core::FileExists(dir + ending)) return dir;
					namespace fs = std::experimental::filesystem;
					fs::directory_iterator endIt;
					for (fs::directory_iterator dirIt(dir); dirIt != endIt; ++dirIt)
					{
						auto child = dirIt->path();
						if (fs::is_directory(child)) queue.push(child.string());
					}
				}
			}
		}
		return{};
	}

	std::string FindRootDirectory(const std::string& rootName, bool searchInChildrenDirs)
	{
		return FindRootDirectory(rootName.c_str(), searchInChildrenDirs);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

using namespace EngineBuildingBlocks;

PathHandler::PathHandler()
{
	m_RootDirectory = FindRootDirectory("Framework", true);

	if(m_RootDirectory.empty())
		EngineBuildingBlocks::RaiseException("Unable to find the root directory.");
}

// Note that this only works for EXISTING paths.
std::string PathHandler::CompletePath(const std::string& path) const
{
	if (Core::IsDirectory(path))
	{
		char lastChar = path[path.length() - 1];
		if (lastChar != '/' && lastChar != '\\')
		{
			return path + "/";
		}
	}
	return path;
}

std::string PathHandler::GetPathFromRootDirectory(const std::string& relativePath) const
{
	return CompletePath(m_RootDirectory + "/" + relativePath);
}