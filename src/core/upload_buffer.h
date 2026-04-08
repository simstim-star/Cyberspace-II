#pragma once
#include <basetsd.h>
#include <d3dx12.h>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace Sendai
{
struct UploadBuffer
{
    ComPtr<ID3D12Resource> Buffer;
    UINT8 *BaseMappedPtr;
    UINT64 Size;
    UINT64 CurrentOffset;
};
} // namespace Sendai