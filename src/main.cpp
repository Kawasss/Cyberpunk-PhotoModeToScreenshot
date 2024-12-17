#include <Windows.h>
#include <ShlObj_core.h>

#include <iostream>
#include <string>
#include <filesystem>

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

std::vector<std::string> GetImage()
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

	hr = dialog->SetFileTypes(1, &filter);
	if (!SUCCEEDED(hr))
		return ret;
	
	std::string dir = GetPicturesDirectory() + "\\Cyberpunk 2077";
	std::wstring wdir = std::wstring(dir.begin(), dir.end());

	IShellItem* pFolder = nullptr;
	hr = SHCreateItemFromParsingName(wdir.c_str(), NULL, IID_PPV_ARGS(&pFolder));
	if (!SUCCEEDED(hr))
		return ret;

	DWORD options = 0;
	hr = dialog->GetOptions(&options);
	if (!SUCCEEDED(hr))
		return ret;

	hr = dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_STRICTFILETYPES);
	if (!SUCCEEDED(hr))
		return ret;

	hr = dialog->SetFolder(pFolder);
	pFolder->Release();
	if (!SUCCEEDED(hr))
		return ret;

	hr = dialog->Show(NULL);
	if (!SUCCEEDED(hr))
		return ret;

	IShellItemArray* items;
	hr = dialog->GetResults(&items);
	if (!SUCCEEDED(hr))
		return ret;

	DWORD count = 0;
	hr = items->GetCount(&count);
	if (!SUCCEEDED(hr))
		return ret;

	if (count == 0)
		return ret;
	ret.reserve(count);

	for (DWORD i = 0; i < count; i++)
	{
		IShellItem* item = nullptr;
		hr = items->GetItemAt(i, &item);
		if (!SUCCEEDED(hr))
			return ret;

		wchar_t* wpath = nullptr;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &wpath);
		if (!SUCCEEDED(hr))
			return ret;

		std::string path = ConvertWideString(wpath);
		ret.emplace_back(path);

		std::cout << path << '\n';
	}
	return ret;
}

int main(int argc, char** argv)
{
	std::vector<std::string> png = GetImage();
	if (png.empty())
		return 1;

	Image image = Image::ReadFromFile(png[0]);
	if (!image.succeeded)
		return 1;
	
	fs::path ssFolder, tbFolder;
	steam::RequestScreenshotDirectory(ssFolder, tbFolder);

	std::string file = steam::CreateScreenshotName();

	std::cout << "Generated steam name '" << file << "'!\n";

	std::string screenshotFile = ssFolder.string() + "\\" + file;
	std::string thumbnailsFile = tbFolder.string() + "\\" + file;

#ifdef _DEBUG
	std::cout << screenshotFile << '\n' << thumbnailsFile << '\n';
#endif

	if (!image.WriteToFile(screenshotFile, thumbnailsFile))
		return 1;

	return 0;
}