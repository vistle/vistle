// EngineBuildingBlocks/PathHandler.h

#ifndef _ENGINEBUILDINGBLOCKS_PATHHANDLER_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_PATHHANDLER_H_INCLUDED_

#include <string>

namespace EngineBuildingBlocks
{
	std::string GetExecutablePath();
	std::string FindRootDirectory(const char* rootName, bool searchInChildrenDirs);
	std::string FindRootDirectory(const std::string& rootName, bool searchInChildrenDirs);

	// This path handler searches for root files.
	class PathHandler
	{
		std::string m_RootDirectory;

		std::string CompletePath(const std::string& path) const;

	public:

		PathHandler();

		std::string GetPathFromRootDirectory(const std::string& relativePath) const;
	};
}

#endif