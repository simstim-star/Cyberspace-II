#include "../core/pch.h"

#include "error.h"
#include <format>
#include <iostream>

VOID ExitIfFailed(const HRESULT hr)
{
    if (!FAILED(hr))
    {
        return;
    }

    std::string ErrorMessage = std::format("ERROR: HRESULT 0x{:08X}\n", static_cast<UINT>(hr));
    OutputDebugStringA(ErrorMessage.c_str());

    if (IsDebuggerPresent())
    {
        __debugbreak();
    }

    exit(EXIT_FAILURE);
}

VOID ExitWithMessage(const std::string_view Message)
{
    std::cerr << "FATAL ERROR: " << Message << std::endl;

    if (IsDebuggerPresent())
    {
        __debugbreak();
    }

    std::exit(EXIT_FAILURE);
}