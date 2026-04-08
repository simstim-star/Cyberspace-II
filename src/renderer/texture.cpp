#include "../core/pch.h"

#include "../error/error.h"
#include "../win32/str_helper.h"
#include "renderer.h"
#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include "../../deps/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../deps/stb_image_resize2.h"
#include "../core/log.h"

#include "../shaders/sendai/shader_defs.h"
#include <d3dx12.h>
#include <filesystem>

namespace fs = std::filesystem;
using Sendai::Textures;

Textures::Textures(ID3D12Device *Device, ID3D12GraphicsCommandList *CommandList)
{
    _TexturesCount = 0;
    _Device = Device;
    _CommandList = CommandList;
    _DescriptorHandleIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = PBR_N_TEXTURES_DESCRIPTORS,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0,
    };
    HRESULT hr = Device->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(_DescriptorHeap.GetAddressOf()));
    ExitIfFailed(hr);
    _DescriptorHeap->SetName(L"TexturesDescHeap");

    D3D12_HEAP_PROPERTIES HeapProps = {.Type = D3D12_HEAP_TYPE_UPLOAD};
    UploadBuffer.Size = MEGABYTES(128);
    UploadBuffer.CurrentOffset = 0;
    auto TextureBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(UploadBuffer.Size);
    hr = Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TextureBufferDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(UploadBuffer.Buffer.GetAddressOf()));
    ExitIfFailed(hr);
    UploadBuffer.Buffer->SetName(L"TextureUploadBuffer");

    D3D12_RANGE Range = {0, 0};
    UploadBuffer.Buffer->Map(0, &Range, (VOID **)&UploadBuffer.BaseMappedPtr);
}

VOID Textures::CreateUITexture(const std::wstring &Path, const UINT nkSlotIndex)
{
    if (Cache.contains(Path))
    {
        return;
    }

    if (!fs::exists(Path))
    {
        Sendai::LOG.Appendf(L"Path %s doesn't exist", Path);
        return;
    }

    INT W = 0, H = 0, Channels = 0;
    std::string PathUTF8(Path.begin(), Path.end());
    UINT8 *Pixels = stbi_load(PathUTF8.c_str(), &W, &H, &Channels, 4);

    if (!Pixels)
    {
        return;
    }

    Sendai::Texture Source = {
        .Width = W,
        .Height = H,
        .Name = Path.c_str(),
        .MipPixels = {Pixels},
        .MipLevels = 1,
    };

    GPUTexture NewTex = {0};
    NewTex.GpuTexture = CommandCreateTextureGPU(Source);

    Cache.try_emplace(Path, NewTex);

    stbi_image_free(Pixels);
}

Sendai::GPUTexture Textures::UploadTexture(const Sendai::Texture &Source)
{
    if (Cache.contains(Source.Name))
    {
        return Cache.at(Source.Name);
    }

    uint32_t SlotIndex = _TexturesCount++;
    GPUTexture NewTex = {
        .GpuTexture = CommandCreateTextureGPU(Source),
        .HeapIndex = SlotIndex,
    };

    D3D12_CPU_DESCRIPTOR_HANDLE CpuDescHandle = _DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    CpuDescHandle.ptr += (SIZE_T)SlotIndex * _DescriptorHandleIncrementSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {.MipLevels = Source.MipLevels},
    };

    _Device->CreateShaderResourceView(NewTex.GpuTexture.Get(), &SrvDesc, CpuDescHandle);
    Cache.try_emplace(Source.Name, NewTex);
    return NewTex;
}

VOID Textures::CreateCustomTexture(const std::wstring &Path)
{
    if (!fs::exists(Path))
    {
        Sendai::LOG.Appendf(L"Path %s doesn't exist", Path);
        return;
    }

    INT W = 0, H = 0, Channels = 0;
    std::string PathUTF8(Path.begin(), Path.end());
    UINT8 *Pixels = stbi_load(PathUTF8.c_str(), &W, &H, &Channels, 4);

    if (!Pixels)
    {
        return;
    }

    Texture Source = {.Width = W, .Height = H, .Name = Path.c_str(), .MipPixels = {Pixels}, .MipLevels = 1};
    UploadTexture(Source);
    stbi_image_free(Pixels);
}

ComPtr<ID3D12Resource> Textures::CommandCreateTextureGPU(const Sendai::Texture &SourceTexture)
{
    auto TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT64)SourceTexture.Width,
                                                (UINT64)SourceTexture.Height, 1, (UINT16)SourceTexture.MipLevels);
    auto HeapDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ComPtr<ID3D12Resource> Texture;
    HRESULT hr =
        _Device->CreateCommittedResource(&HeapDefault, D3D12_HEAP_FLAG_NONE, &TexDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                         NULL, IID_PPV_ARGS(Texture.GetAddressOf()));
    ExitIfFailed(hr);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[D3D12_REQ_MIP_LEVELS];
    UINT NumRows[D3D12_REQ_MIP_LEVELS];
    UINT64 RowSizeInBytes[D3D12_REQ_MIP_LEVELS];
    UINT64 TotalUploadSize = 0;

    _Device->GetCopyableFootprints(&TexDesc, 0, SourceTexture.MipLevels, 0, Layouts, NumRows, RowSizeInBytes,
                                   &TotalUploadSize);

    UINT64 Offset = SuballocateTextureUpload(TotalUploadSize);
    UINT8 *pUploadBufferBase = UploadBuffer.BaseMappedPtr + Offset;

    for (uint32_t MipLevel = 0; MipLevel < SourceTexture.MipLevels; MipLevel++)
    {
        UINT8 *pDestination = pUploadBufferBase + Layouts[MipLevel].Offset;
        UINT8 *pSource = (UINT8 *)SourceTexture.MipPixels[MipLevel];
        for (UINT y = 0; y < NumRows[MipLevel]; y++)
        {
            std::memcpy(pDestination + (y * Layouts[MipLevel].Footprint.RowPitch),
                        pSource + (y * RowSizeInBytes[MipLevel]), RowSizeInBytes[MipLevel]);
        }
        D3D12_TEXTURE_COPY_LOCATION DestinationLocation = {.pResource = Texture.Get(),
                                                           .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                           .SubresourceIndex = MipLevel};
        D3D12_TEXTURE_COPY_LOCATION SourceLocation = {.pResource = UploadBuffer.Buffer.Get(),
                                                      .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                                                      .PlacedFootprint = Layouts[MipLevel]};
        SourceLocation.PlacedFootprint.Offset += Offset;
        _CommandList->CopyTextureRegion(&DestinationLocation, 0, 0, 0, &SourceLocation, nullptr);
    }

    auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    _CommandList->ResourceBarrier(1, &Barrier);

    Texture->SetName(SourceTexture.Name);
    return Texture;
}

UINT64
Textures::SuballocateTextureUpload(const UINT64 Size)
{
    UINT64 AlignedOffset = ROUND_UP_POWER_OF_2(UploadBuffer.CurrentOffset, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    if (AlignedOffset + Size > UploadBuffer.Size)
    {
        UploadBuffer.CurrentOffset = Size;
        return 0;
    }

    UploadBuffer.CurrentOffset = AlignedOffset + Size;
    return AlignedOffset;
}

UINT32
Textures::GetTextureIndex(const Sendai::Texture &Texture)
{
    return UploadTexture(WhiteTexture).HeapIndex;
}

VOID Textures::GenerateMips(Sendai::Model *Model, M_Arena *UploadArena)
{
    for (size_t i = 0; i < Model->Images.size(); ++i)
    {
        Sendai::Texture *Texture = &Model->Images[i];

        if (Texture->MipPixels[0] == NULL || Texture->MipLevels <= 1)
        {
            continue;
        }

        INT CurrentWidth = Texture->Width;
        INT CurrentHeight = Texture->Height;

        for (size_t MipLevel = 1; MipLevel < Texture->MipLevels; MipLevel++)
        {
            INT NextWidth = CurrentWidth > 1 ? CurrentWidth / 2 : 1;
            INT NextHeight = CurrentHeight > 1 ? CurrentHeight / 2 : 1;

            Texture->MipPixels[MipLevel] = (UINT8 *)M_ArenaAlloc(UploadArena, NextWidth * NextHeight * 4);

            stbir_resize_uint8_linear((unsigned char *)Texture->MipPixels[MipLevel - 1], CurrentWidth, CurrentHeight, 0,
                                      (unsigned char *)Texture->MipPixels[MipLevel], NextWidth, NextHeight, 0,
                                      STBIR_RGBA);

            CurrentWidth = NextWidth;
            CurrentHeight = NextHeight;
        }
    }
}