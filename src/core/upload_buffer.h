#pragma once

#include "memory.h"

namespace Sendai
{
class UploadBuffer
{
  public:
    UploadBuffer(UINT64 Size, UINT Alignment)
        : _Alignment{Alignment}, _CurrentOffset{0}, _Size{Size}, _BaseMappedPtr{nullptr}
    {
    }

    inline ID3D12Resource *Get()
    {
        return _Buffer.Get();
    }

    inline ID3D12Resource** GetAddressOf()
    {
        return _Buffer.GetAddressOf();
    }

    inline UINT64 Size() const
    {
        return _Size;
    }

    inline UINT64 CurrentOffset() const
    {
        return _CurrentOffset;
    }

    inline VOID SetCurrentOffset(UINT64 Offset)
    {
        _CurrentOffset = Offset;
    }

    inline UINT8 *CurrentMappedPtr()
    {
        return _BaseMappedPtr + _CurrentOffset;
    }

    inline VOID Map(UINT Subresource = 0, D3D12_RANGE Range = {0, 0})
    {
        _Buffer->Map(Subresource, &Range, (VOID **)&_BaseMappedPtr);
    }

    inline VOID AddOffset(UINT64 Value)
    {
        _CurrentOffset += ROUND_UP_POWER_OF_2(Value, _Alignment);
    }

    template <typename T> inline D3D12_GPU_VIRTUAL_ADDRESS PushBack(const T &Data)
    {
        return _PushBack(&Data, sizeof(T));
    }

    template <typename T> inline D3D12_GPU_VIRTUAL_ADDRESS PushBack(const std::vector<T> &Data)
    {
        return _PushBack(Data.data(), Data.size() * sizeof(T));
    }

  private:
    inline D3D12_GPU_VIRTUAL_ADDRESS _PushBack(const void *Ptr, const size_t Size)
    {
        auto Address = _Buffer->GetGPUVirtualAddress() + _CurrentOffset;
        std::memcpy(_BaseMappedPtr + _CurrentOffset, Ptr, Size);
        AddOffset(Size);
        return Address;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> _Buffer;
    UINT8 *_BaseMappedPtr;
    UINT64 _Size;
    UINT64 _CurrentOffset;
    UINT _Alignment;
};
} // namespace Sendai