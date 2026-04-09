#pragma once
#include "memory.h"
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
    UINT Alignment;

    UploadBuffer(UINT Alignment) : Alignment{Alignment}, CurrentOffset{0}, Size{0}, BaseMappedPtr{nullptr}
    {
    }

    inline VOID Map(UINT Subresource = 0, D3D12_RANGE Range = {0, 0})
    {
        Buffer->Map(Subresource, &Range, (VOID **)&BaseMappedPtr);
    }

    template <typename T> inline D3D12_GPU_VIRTUAL_ADDRESS Insert(const T &Data)
    {
        auto Address = Buffer->GetGPUVirtualAddress() + CurrentOffset;
        std::memcpy(BaseMappedPtr + CurrentOffset, &Data, sizeof(T));
        CurrentOffset += ROUND_UP_POWER_OF_2(sizeof(T), Alignment);
        return Address;
    }
};
} // namespace Sendai