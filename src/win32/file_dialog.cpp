#include "../core/pch.h"

#include "file_dialog.h"
#include <shobjidl.h>

using Microsoft::WRL::ComPtr;

constexpr COMDLG_FILTERSPEC ModelsFilter[] = {
    {L"glTF Models", L"*.gltf;*.glb"}, {L"obj Models", L"*.obj"}, {L"All Files", L"*.*"}};

PWSTR
Win32SelectGLTFPath(VOID)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        return nullptr;
    }

    ComPtr<IFileOpenDialog> FileOpenDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(FileOpenDialog.GetAddressOf()));
    if (FAILED(hr))
    {
        CoUninitialize();
        return nullptr;
    }

    FileOpenDialog->SetFileTypes(ARRAYSIZE(ModelsFilter), ModelsFilter);

    hr = FileOpenDialog->Show(nullptr);
    if (FAILED(hr))
    {
        CoUninitialize();
        return nullptr;
    }

    ComPtr<IShellItem> ChosenFile = nullptr;
    hr = FileOpenDialog->GetResult(&ChosenFile);
    if (FAILED(hr))
    {
        CoUninitialize();
        return nullptr;
    }

    PWSTR FilePath = nullptr;
    hr = ChosenFile->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

    CoUninitialize();

    if (FAILED(hr))
    {
        return nullptr;
    }

    return FilePath;
}