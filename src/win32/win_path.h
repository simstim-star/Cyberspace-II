#pragma once

#include <filesystem>

/******************************************************************************************************************
    Retrieves the path of the executable file of the current process with a last slash ('\\') appended in the end.
    Example: C:\\path\\to\\my\\executable.exe\\
*******************************************************************************************************************/
std::filesystem::path Win32GetModuleDir();

std::wstring Win32CurrPath();

std::wstring Win32FullPath(std::wstring_view subPath);

std::wstring Win32GetFileNameOnly(std::wstring_view fullPath);

std::string Win32AppendFileNameToPath(std::wstring_view basePath, std::string_view fileName);

VOID Win32RemoveAllAfterLastSlash(std::wstring &pathBuffer);
