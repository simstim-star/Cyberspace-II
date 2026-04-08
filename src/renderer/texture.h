#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include "../core/upload_buffer.h"

struct M_Arena;

constexpr UINT8 WHITE_PIXEL[] = {255, 255, 255, 255};

namespace Sendai
{

struct Renderer;
struct Model;

struct Texture
{
    INT Width;
    INT Height;
    INT Channels;
    const wchar_t *Name;
    size_t Size;
    std::array<const UINT8 *, D3D12_REQ_MIP_LEVELS> MipPixels;
    UINT MipLevels;
};

constexpr Texture WhiteTexture = {
    .Width = 1,
    .Height = 1,
    .Name = L"fallback_white",
    .Size = 4,
    .MipPixels = {WHITE_PIXEL},
    .MipLevels = 1,
};

struct GPUTexture
{
    ComPtr<ID3D12Resource> GpuTexture;
    UINT HeapIndex{0};
};

class Textures
{
  public:
    Textures(ID3D12Device *Device, ID3D12GraphicsCommandList *CommandList);

    VOID CreateCustomTexture(const std::wstring &Path);

    VOID CreateUITexture(const std::wstring &Path, const UINT nkSlotIndex);

    GPUTexture UploadTexture(const Sendai::Texture &Source);

    ComPtr<ID3D12Resource> CommandCreateTextureGPU(const Sendai::Texture &SourceTexture);

    UINT64 SuballocateTextureUpload(const UINT64 Size);

    UINT32
    GetTextureIndex(const Sendai::Texture &Texture);

    VOID GenerateMips(Sendai::Model *Model, M_Arena *UploadArena);

    inline ID3D12DescriptorHeap *GetHeap()
    {
        return _DescriptorHeap.Get();
    }

    inline D3D12_GPU_DESCRIPTOR_HANDLE GetHeapDescriptorHandle()
    {
        return _DescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart();
    }

    static inline UINT CalculateMipLevels(INT Width, INT Height)
    {
        return 1 + (uint32_t)floorf(log2f((float)max(Width, Height)));
    }

  private:
    ID3D12Device *_Device;
    // I don't like to share the same command list for everything...
    ID3D12GraphicsCommandList *_CommandList;
    ComPtr<ID3D12DescriptorHeap> _DescriptorHeap;
    UINT _TexturesCount;
    UploadBuffer UploadBuffer;
    std::unordered_map<std::wstring, GPUTexture> Cache;
    UINT _DescriptorHandleIncrementSize;
};
} // namespace Sendai
