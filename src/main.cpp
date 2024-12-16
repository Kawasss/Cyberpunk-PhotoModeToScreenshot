#include <Windows.h>
#include <ShlObj_core.h>

#include <iostream>
#include <string>
#include <filesystem>

#include "DownSampler.h"
#include "Image.h"
#include "Steam.h"

namespace fs = std::filesystem;

std::string ConvertWideString(const wchar_t* wstring)
{
	std::string str;
	for (int i = 0; wstring[i] != L'\0'; i++)
		str += static_cast<char>(wstring[i]);
	return str;
}

std::string GetPicturesDirectory()
{
	wchar_t* path;
	SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &path);
	std::string str = ConvertWideString(path);

	CoTaskMemFree(path);
	return str;
}

std::string GetImage()
{
	OPENFILENAMEA ofn;
	char szFile[MAX_PATH] = { 0 };

	std::string dir = GetPicturesDirectory() + "\\Cyberpunk 2077";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "png\0*.png\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrInitialDir = dir.c_str();
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	return GetOpenFileNameA(&ofn) == TRUE ? ofn.lpstrFile : "";
}

int main(int argc, char** argv)
{
	std::string png = GetImage();
	if (png.empty())
		return 1;

	Image image = Image::ReadFromFile(png);
	if (!image.succeeded)
		return 1;
	
	fs::path ssFolder, tbFolder;
	steam::RequestScreenshotDirectory(ssFolder, tbFolder);

	std::string file = steam::CreateScreenshotName();

	std::cout << "Generated steam name '" << file << "'!\n";

	std::string screenshotFile = ssFolder.string() + "\\" + file;
	std::string thumbnailsFile = tbFolder.string() + "\\" + file;

	//std::cout << screenshotFile << '\n' << thumbnailsFile << '\n';

	if (!image.WriteToFile(screenshotFile, thumbnailsFile))
		return 1;

	return 0;
}