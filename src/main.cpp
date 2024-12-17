#include <Windows.h>
#include <ShlObj_core.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <execution>

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
	wchar_t* path;
	SHGetKnownFolderPath(FOLDERID_Pictures, 0, NULL, &path);
	std::string str = ConvertWideString(path);

	CoTaskMemFree(path);
	return str;
}

std::vector<std::string> GetImages()
{
	COMDLG_FILTERSPEC filter = { L"PNG image", L"*.png;*.PNG" };
	std::vector<std::string> ret;

	DialogManager dialog;
	HRESULT hr = CoInitialize(NULL);
	if (!SUCCEEDED(hr))
		return ret;

	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog.ptr));
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

int main(int argc, char** argv)
{
	std::vector<std::string> pngs = GetImages();
	if (pngs.empty())
		return 1;

	fs::path ssFolder, tbFolder;
	steam::RequestScreenshotDirectory(ssFolder, tbFolder);

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

			#ifdef _DEBUG
			std::cout << screenshotFile << '\n' << thumbnailsFile << '\n';
			#endif

			if (!image.WriteToFile(screenshotFile, thumbnailsFile))
				return;
		}
	);
	return 0;
}