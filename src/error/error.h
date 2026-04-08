#pragma once

#include <string_view>

VOID ExitIfFailed(const HRESULT hr);
VOID ExitWithMessage(const std::string_view Message);