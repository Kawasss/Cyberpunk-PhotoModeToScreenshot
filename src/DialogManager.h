#pragma once
#include <shobjidl_core.h>

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