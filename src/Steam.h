#pragma once
#include <string>

namespace std
{
	namespace filesystem
	{
		class path;
	}
}
namespace fs = std::filesystem;

namespace steam
{
	extern std::string CreateScreenshotName(int index = 1);
	extern void        RequestScreenshotDirectory(fs::path& screenshot, fs::path& thumbnails);
}