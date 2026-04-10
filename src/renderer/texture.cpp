#include "../core/pch.h"

#include "../error/error.h"
#include "renderer.h"
#include "texture.h"

#include "../shaders/sendai/shader_defs.h"

#include <DirectXTex.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3dx12_core.h>
#include <d3dx12_barriers.h>

namespace fs = std::filesystem;
using namespace Microsoft::WRL;
using Sendai::Textures;

Textures::Textures(ID3D12Device *Device, ID3D12GraphicsCommandList *CommandList)
    : _UploadBuffer{MEGABYTES(512), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT}
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
    auto TextureBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(_UploadBuffer.Size());
    hr = Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TextureBufferDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(_UploadBuffer.GetAddressOf()));
    ExitIfFailed(hr);
    _UploadBuffer.Map();
}

UINT Textures::Load(ID3D12Device *Device, const std::wstring &Path)
{
    if (_Cache.contains(Path))
    {
        return _Cache.at(Path).HeapIndex;
    }

    HRESULT hr;
    DirectX::ScratchImage Image;
    DirectX::TexMetadata Metadata;

    std::wstring Extension = fs::path(Path).extension().wstring();
    std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::toupper);

    if (Extension == L".DDS")
    {
        hr = DirectX::LoadFromDDSFile(Path.c_str(), DirectX::DDS_FLAGS_NONE, &Metadata, Image);
    }
    else if (Extension == L".TGA")
    {
        hr = DirectX::LoadFromTGAFile(Path.c_str(), &Metadata, Image);
    }
    else
    {
        hr = DirectX::LoadFromWICFile(Path.c_str(), DirectX::WIC_FLAGS_NONE, &Metadata, Image);
    }

    if (FAILED(hr))
    {
        return 0;
    }

    if (Metadata.mipLevels == 1 && !DirectX::IsCompressed(Metadata.format))
    {
        DirectX::ScratchImage ImageWithMips;
        hr = DirectX::GenerateMipMaps(Image.GetImages(), Image.GetImageCount(), Metadata, DirectX::TEX_FILTER_DEFAULT,
                                      0, ImageWithMips);
        if (SUCCEEDED(hr))
        {
            Image = std::move(ImageWithMips);
            Metadata = Image.GetMetadata();
        }
    }

    Sendai::Texture Source = {
        .Name = Path.c_str(),
        .Width = (INT)Metadata.width,
        .Height = (INT)Metadata.height,
        .MipLevels = (UINT)Metadata.mipLevels,
        .Format = Metadata.format,
    };

    for (uint32_t i = 0; i < Source.MipLevels; ++i)
    {
        const DirectX::Image *img = Image.GetImage(i, 0, 0);
        Source.MipPixels[i] = img->pixels;
        Source.MipRowPitches[i] = img->rowPitch;
    }
    GPUTexture NewTex = _UploadToGPU(Source);
    return NewTex.HeapIndex;
}

UINT Textures::LoadEmbedded(aiTexture *pEmbeddedTexture)
{
    std::wstring Key = L"Embedded_" + std::to_wstring((uintptr_t)pEmbeddedTexture);
    if (_Cache.contains(Key))
    {
        return _Cache.at(Key).HeapIndex;
    }

    DirectX::TexMetadata Metadata;
    DirectX::ScratchImage ScratchImage;

    HRESULT hr = DirectX::LoadFromWICMemory(reinterpret_cast<uint8_t *>(pEmbeddedTexture->pcData),
                                            pEmbeddedTexture->mWidth, DirectX::WIC_FLAGS_NONE, &Metadata, ScratchImage);

    if (FAILED(hr))
    {
        return 0;
    }

    D3D12_RESOURCE_DESC TextureDesc =
        CD3DX12_RESOURCE_DESC::Tex2D(Metadata.format, static_cast<UINT64>(Metadata.width),
                                     static_cast<UINT64>(Metadata.height), 1, static_cast<UINT16>(Metadata.mipLevels));

    UINT64 TotalSize = 0;
    _Device->GetCopyableFootprints(&TextureDesc, 0, (UINT)Metadata.mipLevels, 0, nullptr, nullptr, nullptr, &TotalSize);

    Sendai::Texture Source = {
        .Name = Key.c_str(),
        .Width = static_cast<INT>(Metadata.width),
        .Height = static_cast<INT>(Metadata.height),
        .Size = static_cast<size_t>(TotalSize),
        .MipLevels = 1,
        .Format = Metadata.format,
    };
    const DirectX::Image *pImage = ScratchImage.GetImage(0, 0, 0);
    Source.MipPixels[0] = pImage->pixels;
    Source.MipRowPitches[0] = pImage->rowPitch;
    return _UploadToGPU(Source).HeapIndex;
}

Sendai::GPUTexture Textures::_UploadToGPU(const Sendai::Texture &TextureToUpload)
{
    if (_Cache.contains(TextureToUpload.Name))
    {
        return _Cache.at(TextureToUpload.Name);
    }

    uint32_t SlotIndex = _TexturesCount++;
    GPUTexture NewTex = {
        .GpuTexture = _CommandCreateTextureGPU(TextureToUpload),
        .HeapIndex = SlotIndex,
    };

    D3D12_CPU_DESCRIPTOR_HANDLE CpuDescHandle = _DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    CpuDescHandle.ptr += (SIZE_T)SlotIndex * _DescriptorHandleIncrementSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {
        .Format = TextureToUpload.Format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {.MipLevels = TextureToUpload.MipLevels},
    };

    _Device->CreateShaderResourceView(NewTex.GpuTexture.Get(), &SrvDesc, CpuDescHandle);
    _Cache.try_emplace(TextureToUpload.Name, NewTex);
    return NewTex;
}

ComPtr<ID3D12Resource> Textures::_CommandCreateTextureGPU(const Sendai::Texture &SourceTexture)
{
    ComPtr<ID3D12Resource> Texture;
    auto HeapDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(SourceTexture.Format, (UINT64)SourceTexture.Width,
                                                (UINT64)SourceTexture.Height, 1, (UINT16)SourceTexture.MipLevels);
    HRESULT hr =
        _Device->CreateCommittedResource(&HeapDefault, D3D12_HEAP_FLAG_NONE, &TexDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                         nullptr, IID_PPV_ARGS(Texture.GetAddressOf()));
    ExitIfFailed(hr);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[D3D12_REQ_MIP_LEVELS];
    UINT NumRows[D3D12_REQ_MIP_LEVELS];
    UINT64 RowSizeInBytes[D3D12_REQ_MIP_LEVELS];
    UINT64 TotalUploadSize = 0;

    _Device->GetCopyableFootprints(&TexDesc, 0, SourceTexture.MipLevels, 0, Layouts, NumRows, RowSizeInBytes,
                                   &TotalUploadSize);

    UINT64 CurrentBaseOffset = _UploadBuffer.CurrentOffset();
    for (uint32_t MipLevel = 0; MipLevel < SourceTexture.MipLevels; MipLevel++)
    {
        UINT8 *pDestination = _UploadBuffer.CurrentMappedPtr() + Layouts[MipLevel].Offset;
        UINT8 *pSource = (UINT8 *)SourceTexture.MipPixels[MipLevel];
        for (UINT y = 0; y < NumRows[MipLevel]; y++)
        {
            UINT8 *pDestRow = pDestination + (y * Layouts[MipLevel].Footprint.RowPitch);
            UINT8 *pSrcRow = pSource + (y * SourceTexture.MipRowPitches[MipLevel]);
            std::memcpy(pDestRow, pSrcRow, RowSizeInBytes[MipLevel]);
        }

        D3D12_TEXTURE_COPY_LOCATION DestinationLocation = {.pResource = Texture.Get(),
                                                           .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                           .SubresourceIndex = MipLevel};
        D3D12_TEXTURE_COPY_LOCATION SourceLocation = {.pResource = _UploadBuffer.Get(),
                                                      .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                                                      .PlacedFootprint = Layouts[MipLevel]};
        SourceLocation.PlacedFootprint.Offset += CurrentBaseOffset;
        _CommandList->CopyTextureRegion(&DestinationLocation, 0, 0, 0, &SourceLocation, nullptr);
    }
    _UploadBuffer.AddOffset(TotalUploadSize);

    auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    _CommandList->ResourceBarrier(1, &Barrier);

    Texture->SetName(SourceTexture.Name);
    return Texture;
}
