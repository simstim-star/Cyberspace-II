#include "../core/pch.h"

namespace fs = std::filesystem;

fs::path Win32GetModuleDir()
{
    wchar_t buffer[MAX_PATH];
    DWORD size = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (size == 0 || size == MAX_PATH)
    {
        std::exit(EXIT_FAILURE);
    }
    return fs::path(buffer).parent_path() / "";
}

std::wstring Win32CurrPath()
{
    return Win32GetModuleDir().wstring();
}

std::wstring Win32FullPath(std::wstring_view subPath)
{
    if (!subPath.empty() && (subPath[0] == L'/' || subPath[0] == L'\\'))
    {
        subPath.remove_prefix(1);
    }
    return (Win32GetModuleDir() / subPath).wstring();
}

std::wstring Win32GetFileNameOnly(std::wstring_view fullPath)
{
    fs::path p(fullPath);
    return p.stem().wstring();
}

std::string Win32AppendFileNameToPath(std::wstring_view basePath, std::string_view fileName)
{
    fs::path p(basePath);
    p /= fileName;

    return p.string();
}

VOID Win32RemoveAllAfterLastSlash(std::wstring &pathBuffer)
{
    fs::path p(pathBuffer);
    if (p.has_parent_path())
    {
        pathBuffer = (p.parent_path() / "").wstring();
    }
    else
    {
        pathBuffer.clear();
    }
}