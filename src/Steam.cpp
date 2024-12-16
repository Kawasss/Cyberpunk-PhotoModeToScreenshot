#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

#include <shobjidl_core.h>

#include "Steam.h"

class DialogManager
{
public:
	DialogManager() = default;
	~DialogManager() { if (ptr) ptr->Release(); }

	DialogManager(const DialogManager&) = delete;
	DialogManager& operator=(DialogManager&&) = delete;

	IFileOpenDialog* operator->() { return ptr; }

	IFileOpenDialog* ptr = nullptr;
};

inline std::string ConvertWideStringToString(const wchar_t* wstring)
{
	std::string str;
	for (int i = 0; wstring[i] != L'\0'; i++)
		str += static_cast<char>(wstring[i]);
	return str;
}

namespace steam
{
	std::string CreateScreenshotName(int index)
	{
		std::tm local;
		std::time_t now = std::time(nullptr);
		if (localtime_s(&local, &now))
			return "00000000000000_" + std::to_string(index) + ".jpg"; // placeholder name

		std::stringstream stream;                                      // steams screenshot naming goes like this: yymmddhhmmss_x
		stream << std::setfill('0')
			<< std::setw(4) << 1900 + local.tm_year                    // std::tm start counting from the year 1900
			<< std::setw(2) << local.tm_mon + 1                        // the months are given with a starting index of 0, but steam uses 1
			<< std::setw(2) << local.tm_mday
			<< std::setw(2) << local.tm_hour
			<< std::setw(2) << local.tm_min
			<< std::setw(2) << local.tm_sec
			<< '_' << index << ".jpg";

		return stream.str();
	}

	std::string RequestDirectory()
	{
		DialogManager dialog;
		HRESULT res = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog.ptr));
		if (!SUCCEEDED(res))
			return "";

		DWORD options = 0;
		res = dialog->GetOptions(&options);
		if (!SUCCEEDED(res))
			return "";

		options |= FOS_PICKFOLDERS;
		res = dialog->SetOptions(options);
		if (!SUCCEEDED(res))
			return "";

		res = dialog->Show(NULL);
		if (!SUCCEEDED(res))
			return "";

		IShellItem* item = nullptr;
		res = dialog->GetResult(&item);
		if (!SUCCEEDED(res))
			return "";

		wchar_t* path = nullptr;
		item->GetDisplayName(SIGDN_FILESYSPATH, &path);
		if (!SUCCEEDED(res) || path == nullptr)
			return "";

		std::string ret = ConvertWideStringToString(path);

		item->Release();
		CoTaskMemFree(path);

		return ret;
	}

	void RequestScreenshotDirectory(fs::path& screenshot, fs::path& thumbnails)
	{
		do
		{
			std::cout << "Requesting the steam screenshot folder...\n";

			screenshot = RequestDirectory();
			thumbnails = screenshot.string() + "\\thumbnails";

			std::cout << "Verifying folder integrity...\n";
		} while (!fs::is_directory(thumbnails));

		std::cout << "Found the steam screenshot and thumbnail folder at '" << screenshot.string() << "'!\n";
	}
}