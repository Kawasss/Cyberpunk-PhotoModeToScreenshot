#include <Windows.h>
#include <ShlObj_core.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <execution>
#include <fstream>

#include "DialogManager.h"
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
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, path);
	return path;
}

std::string GetAppDataDirectory()
{
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
	return path;
}

std::vector<std::string> GetImages()
{
	COMDLG_FILTERSPEC filter = { L"PNG image", L"*.png;*.PNG" };
	std::vector<std::string> ret;
	DialogManager dialog;

	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog.ptr));
	if (!SUCCEEDED(hr))
		return ret;

	dialog->SetFileTypes(1, &filter);
	
	std::string dir = GetPicturesDirectory() + "\\Cyberpunk 2077";
	std::wstring wdir = std::wstring(dir.begin(), dir.end());

	IShellItem* pFolder = nullptr;
	hr = SHCreateItemFromParsingName(wdir.c_str(), NULL, IID_PPV_ARGS(&pFolder));
	if (!SUCCEEDED(hr))
		return ret;

	DWORD options = 0;
	dialog->GetOptions(&options);
	dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_STRICTFILETYPES);

	dialog->SetFolder(pFolder);
	pFolder->Release();

	dialog->Show(NULL);
	
	IShellItemArray* items;
	dialog->GetResults(&items);

	DWORD count = 0;
	items->GetCount(&count);

	if (count == 0)
	{
		items->Release();
		return ret;
	}
	ret.reserve(count);

	for (DWORD i = 0; i < count; i++)
	{
		wchar_t* wpath   = nullptr;
		IShellItem* item = nullptr; // i dont know if i have to release these items too ??

		items->GetItemAt(i, &item);
		item->GetDisplayName(SIGDN_FILESYSPATH, &wpath);

		std::string path = ConvertWideString(wpath);
		ret.emplace_back(path);
	}
	items->Release();
	return ret;
}

void WriteDirectoriesToSaveFile(const fs::path& dst, const fs::path& ssFolder, const fs::path& tbFolder)
{
	fs::path parent = dst.parent_path();
	if (!fs::exists(parent))
		fs::create_directory(parent);

	std::ofstream stream(dst.string());
	stream << ssFolder.string();

	std::cout << "Saved screenshot location to the disk!\n";
}

 bool ReadDirectoriesFromSaveFile(const fs::path& src, fs::path& ssFolder, fs::path& tbFolder)
{
	 std::ifstream stream(src.string());
	 std::string ssPath;
	 
	 stream >> ssPath;

	 ssFolder = ssPath;
	 tbFolder = ssPath + "\\thumbnails";

	 std::cout << "Read screenshot location from save file, verifying directories...\n";

	 return fs::exists(ssFolder) && fs::exists(tbFolder);
}

int main(int argc, char** argv)
{
	HRESULT hr = CoInitialize(NULL);
	if (!SUCCEEDED(hr))
		return 1;

	std::cout << "Requesting image(s) to be converted...\n";
	std::vector<std::string> pngs = GetImages();
	if (pngs.empty())
		return 1;

	fs::path saveDir = GetAppDataDirectory() + "\\cp2077_photomode_converter\\directories.txt";
	bool saved = fs::exists(saveDir);

	fs::path ssFolder, tbFolder;
	if (!saved)
	{
		std::cout << "Save file not found! Looked for '" << saveDir.string() << "'\n";

		steam::RequestScreenshotDirectory(ssFolder, tbFolder);
		WriteDirectoriesToSaveFile(saveDir, ssFolder, tbFolder);
	}
	else
	{
		if (!ReadDirectoriesFromSaveFile(saveDir, ssFolder, tbFolder)) // default back to requesting to the directory
		{
			std::cout << "Failed to verify the screenshot location from the save file!\n";

			steam::RequestScreenshotDirectory(ssFolder, tbFolder);
			WriteDirectoriesToSaveFile(saveDir, ssFolder, tbFolder);
		}
	}

	std::for_each(std::execution::par_unseq, pngs.begin(), pngs.end(),
		[&](const std::string& png)
		{
			auto it = std::find(pngs.begin(), pngs.end(), png);
			int index = static_cast<int>(it - pngs.begin());

			Image image = Image::ReadFromFile(pngs[index]);
			if (!image.succeeded)
				return;

			std::string file = steam::CreateScreenshotName(index + 1);

			std::cout << "Generated steam name '" << file << "'!\n";

			std::string screenshotFile = ssFolder.string() + "\\" + file;
			std::string thumbnailsFile = tbFolder.string() + "\\" + file;

			//if (!image.WriteToFile(screenshotFile, thumbnailsFile))
			//	return;
		}
	);
	return 0;
}