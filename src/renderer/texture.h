#pragma once

#include "../core/upload_buffer.h"

#include <DirectXTex.h>

class aiTexture;

namespace Sendai
{

struct Renderer;
struct Model;

struct Texture
{
    const wchar_t *Name;
    INT Width;
    INT Height;
    INT Channels;
    size_t Size;
    std::array<const UINT8 *, D3D12_REQ_MIP_LEVELS> MipPixels;
    std::array<size_t, D3D12_REQ_MIP_LEVELS> MipRowPitches;
    UINT MipLevels;
    DXGI_FORMAT Format;
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

    UINT Load(ID3D12Device *Device, const std::wstring &Path);
    UINT LoadEmbedded(aiTexture *pTexture);


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

    // One day I will use various CommandLists, for now only one for everything
    ID3D12GraphicsCommandList *_CommandList;

    ComPtr<ID3D12DescriptorHeap> _DescriptorHeap;
    UINT _TexturesCount;
    UploadBuffer _UploadBuffer;
    std::unordered_map<std::wstring, GPUTexture> _Cache;
    UINT _DescriptorHandleIncrementSize;

    GPUTexture _UploadToGPU(const Sendai::Texture &TextureToUpload);
    ComPtr<ID3D12Resource> _CommandCreateTextureGPU(const Sendai::Texture &SourceTexture);
    UINT64 _IncrementBufferOffset(const UINT64 Size);
};
} // namespace Sendai
