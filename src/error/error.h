#pragma once

#include <string_view>

void ExitIfFailed(const HRESULT hr);
void ExitWithMessage(const std::string_view Message);