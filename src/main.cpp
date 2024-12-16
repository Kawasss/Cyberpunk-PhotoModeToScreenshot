#include <Windows.h>
#include <ShlObj_core.h>
#include <shobjidl_core.h>

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <chrono>

#include "DownSampler.h"
#include "Image.h"

namespace fs = std::filesystem;

inline std::string ConvertWideString(const wchar_t* wstring)
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

std::string GetScreenShotDirectory() // @TODO: a class to hold the 'dialog' and that will destroy it when its out of scope
{
	IFileOpenDialog* dialog = nullptr;
	HRESULT res = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if (!SUCCEEDED(res))
		return "";

	DWORD options = 0;
	res = dialog->GetOptions(&options);
	if (!SUCCEEDED(res))
	{
		dialog->Release();
		return "";
	}
		
	options |= FOS_PICKFOLDERS;
	res = dialog->SetOptions(options);
	if (!SUCCEEDED(res))
	{
		dialog->Release();
		return "";
	}

	res = dialog->Show(NULL);
	if (!SUCCEEDED(res))
	{
		dialog->Release();
		return "";
	}

	IShellItem* item = nullptr;
	res = dialog->GetResult(&item);
	if (!SUCCEEDED(res))
	{
		dialog->Release();
		return "";
	}

	wchar_t* path = nullptr;
	item->GetDisplayName(SIGDN_FILESYSPATH, &path);
	if (!SUCCEEDED(res) || path == nullptr)
	{
		dialog->Release();
		return "";
	}

	std::string ret = ConvertWideString(path);

	item->Release();
	dialog->Release();
	CoTaskMemFree(path);

	return ret;
}

void GetSteamScreenshotFolder(fs::path& screenshot, fs::path& thumbnails)
{
	do
	{
		std::cout << "Requesting the steam screenshot folder...\n";

		screenshot = GetScreenShotDirectory();
		thumbnails = screenshot.string() + "\\thumbnails";

		std::cout << "Verifying folder integrity...\n";
	}
	while (!fs::is_directory(thumbnails));

	std::cout << "Found the steam screenshot and thumbnail folder at '" << screenshot.string() << "'!\n";
}

std::string GenerateValidScreenshotName(int index = 1)
{
	std::tm local;
	std::time_t now = std::time(nullptr);
	if (localtime_s(&local, &now))
		return "00000000000000_" + std::to_string(index) + ".jpg"; // placeholder name
	
	std::stringstream stream;                                      // steams screenshot naming goes like this: yymmddhhmmss_x
	stream  << std::setfill('0') 
		<< std::setw(4) << 1900 + local.tm_year                    // std::tm start counting from the year 1900
		<< std::setw(2) << local.tm_mon + 1                        // the months are given with a starting index of 0, but steam uses 1
		<< std::setw(2) << local.tm_mday
		<< std::setw(2) << local.tm_hour
		<< std::setw(2) << local.tm_min
		<< std::setw(2) << local.tm_sec
		<< '_' << index << ".jpg";

	return stream.str();
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
	GetSteamScreenshotFolder(ssFolder, tbFolder);

	std::string file = GenerateValidScreenshotName();

	std::cout << "Generated steam name '" << file << "'!\n";

	std::string screenshotFile = ssFolder.string() + "\\" + file;
	std::string thumbnailsFile = tbFolder.string() + "\\" + file;

	if (!image.WriteToFile(screenshotFile, thumbnailsFile))
		return 1;

	return 0;
}