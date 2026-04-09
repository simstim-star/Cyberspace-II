#include "../core/pch.h"

#include "file_dialog.h"
#include <shobjidl.h>

using Microsoft::WRL::ComPtr;

constexpr COMDLG_FILTERSPEC ModelsFilter[] = {
    {L"glTF Models", L"*.gltf;*.glb"}, {L"obj Models", L"*.obj"}, {L"All Files", L"*.*"}};

PWSTR
Win32SelectGLTFPath(VOID)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        return NULL;
    }

    ComPtr<IFileOpenDialog> FileOpenDialog = NULL;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(FileOpenDialog.GetAddressOf()));
    if (FAILED(hr))
    {
        CoUninitialize();
        return NULL;
    }

    FileOpenDialog->SetFileTypes(ARRAYSIZE(ModelsFilter), ModelsFilter);

    hr = FileOpenDialog->Show(NULL);
    if (FAILED(hr))
    {
        CoUninitialize();
        return NULL;
    }

    ComPtr<IShellItem> ChosenFile = NULL;
    hr = FileOpenDialog->GetResult(&ChosenFile);
    if (FAILED(hr))
    {
        CoUninitialize();
        return NULL;
    }

    PWSTR FilePath = NULL;
    hr = ChosenFile->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

    CoUninitialize();

    if (FAILED(hr))
    {
        return NULL;
    }

    return FilePath;
}