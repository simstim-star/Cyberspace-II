#include "../core/pch.h"

#include "../error/error.h"
#include "../win32/str_helper.h"
#include "renderer.h"
#include "texture.h"

#define STB_DS_IMPLEMENTATION
#include "../../deps/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../../deps/stb_image_resize2.h"

static const UINT8 WHITE_PIXEL[] = {255, 255, 255, 255};

static const R_Texture WhiteTexture = {
  .Width = 1,
  .Height = 1,
  .Name = L"fallback_white",
  .MipPixels = {WHITE_PIXEL},
  .MipLevels = 1,
};

void
R_CreateUITexture(std::wstring &Path, R_Core *Renderer, UINT nkSlotIndex)
{
	if (Renderer->Textures.contains(Path)) {
		return;
	}

	INT W, H;
	FILE *f = _wfopen(Path.c_str(), L"rb");
	if (!f) {
		return;
	}

	UINT8 *Pixels = stbi_load_from_file(f, &W, &H, NULL, 4);
	fclose(f);

	R_Texture Source = {
	  .Width = W,
	  .Height = H,
	  .Name = Path,
	  .MipPixels = {Pixels},
	  .MipLevels = 1,
	};

	GPUTexture NewTex = {0};
	NewTex.GpuTexture = R_CommandCreateTextureGPU(Renderer, &Source);
	//UI_SetTextureInNkHeap(nkSlotIndex, NewTex.GpuTexture);

	Renderer->Textures.insert({Path, NewTex});

	stbi_image_free(Pixels);
}

GPUTexture
R_UploadTexture(R_Core *const Renderer, const R_Texture *const Source)
{
	if (Renderer->Textures.contains(Source->Name)) {
		return Renderer->Textures[Source->Name];
	}

	uint32_t SlotIndex = Renderer->TexturesCount++;
	GPUTexture NewTex = {
	  .GpuTexture = R_CommandCreateTextureGPU(Renderer, Source),
	  .HeapIndex = SlotIndex,
	};

	D3D12_CPU_DESCRIPTOR_HANDLE CpuDescHandle = Renderer->TexturesHeap->GetCPUDescriptorHandleForHeapStart();
	CpuDescHandle.ptr += (SIZE_T)SlotIndex * Renderer->DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {
	  .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	  .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
	  .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	  .Texture2D = {.MipLevels = Source->MipLevels},
	};

	Renderer->Device->CreateShaderResourceView(NewTex.GpuTexture.Get(), &SrvDesc, CpuDescHandle);
	Renderer->Textures.insert({Source->Name, NewTex});
	return NewTex;
}

void
R_CreateCustomTexture(std::wstring &Path, R_Core *Renderer)
{
	INT W, H;
	FILE *f = _wfopen(Path.c_str(), L"rb");
	if (!f) {
		return;
	}
	UINT8 *Pixels = stbi_load_from_file(f, &W, &H, NULL, 4);
	fclose(f);
	R_Texture Source = {.Width = W, .Height = H, .Name = Path, .MipPixels = {Pixels}, .MipLevels = 1};
	R_UploadTexture(Renderer, &Source);
	stbi_image_free(Pixels);
}

ComPtr<ID3D12Resource>
R_CommandCreateTextureGPU(R_Core *const Renderer, const R_Texture *const SourceTexture)
{
	D3D12_RESOURCE_DESC TexDesc = {
	  .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
	  .Width = (UINT64)SourceTexture->Width,
	  .Height = (UINT64)SourceTexture->Height,
	  .DepthOrArraySize = 1,
	  .MipLevels = (UINT16)SourceTexture->MipLevels,
	  .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	  .SampleDesc = {1, 0},
	  .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
	  .Flags = D3D12_RESOURCE_FLAG_NONE,
	};
	ComPtr<ID3D12Resource> Texture;
	D3D12_HEAP_PROPERTIES HeapDefault = {.Type = D3D12_HEAP_TYPE_DEFAULT};
	HRESULT hr = Renderer->Device->CreateCommittedResource(&HeapDefault, D3D12_HEAP_FLAG_NONE, &TexDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL,
														   IID_PPV_ARGS(Texture.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[D3D12_REQ_MIP_LEVELS];
	UINT NumRows[D3D12_REQ_MIP_LEVELS];
	UINT64 RowSizeInBytes[D3D12_REQ_MIP_LEVELS];
	UINT64 TotalUploadSize = 0;

	Renderer->Device->GetCopyableFootprints(&TexDesc, 0, SourceTexture->MipLevels, 0, Layouts, NumRows, RowSizeInBytes,
									   &TotalUploadSize);

	UINT64 Offset = R_SuballocateTextureUpload(Renderer, TotalUploadSize);
	UINT8 *pUploadBufferBase = Renderer->TextureUploadBuffer.BaseMappedPtr + Offset;

	for (uint32_t MipLevel = 0; MipLevel < SourceTexture->MipLevels; MipLevel++) {
		UINT8 *pDestination = pUploadBufferBase + Layouts[MipLevel].Offset;
		UINT8 *pSource = (UINT8 *)SourceTexture->MipPixels[MipLevel];
		for (UINT y = 0; y < NumRows[MipLevel]; y++) {
			memcpy(pDestination + (y * Layouts[MipLevel].Footprint.RowPitch), pSource + (y * RowSizeInBytes[MipLevel]),
				   RowSizeInBytes[MipLevel]);
		}
		D3D12_TEXTURE_COPY_LOCATION DestinationLocation = {
		  .pResource = Texture.Get(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, .SubresourceIndex = MipLevel};
		D3D12_TEXTURE_COPY_LOCATION SourceLocation = {.pResource = Renderer->TextureUploadBuffer.Buffer.Get(),
													  .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
													  .PlacedFootprint = Layouts[MipLevel]};
		SourceLocation.PlacedFootprint.Offset += Offset;
		Renderer->CommandList->CopyTextureRegion(&DestinationLocation, 0, 0, 0, &SourceLocation, NULL);
	}

	D3D12_RESOURCE_BARRIER Barrier = {.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
									  .Transition = {
										.pResource = Texture.Get(),
										.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
										.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
										.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
									  }};
	Renderer->CommandList->ResourceBarrier(1, &Barrier);

	Texture->SetName(SourceTexture->Name.c_str());
	return Texture;
}

UINT64
R_SuballocateTextureUpload(R_Core *const Renderer, UINT64 Size)
{
	UINT64 AlignedOffset = ROUND_UP_POWER_OF_2(Renderer->TextureUploadBuffer.CurrentOffset, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

	if (AlignedOffset + Size > Renderer->TextureUploadBuffer.Size) {
		R_ExecuteCommands(Renderer);
		Renderer->TextureUploadBuffer.CurrentOffset = Size;
		return 0;
	}

	Renderer->TextureUploadBuffer.CurrentOffset = AlignedOffset + Size;
	return AlignedOffset;
}

UINT32
R_GetTextureIndex(R_Core *const Renderer, const R_Texture *const Texture)
{
	R_Texture Target;
	if (Texture && !Texture->Name.empty()) {
		Target = *Texture;
	} else {
		Target = WhiteTexture;
	}

	GPUTexture Tex = R_UploadTexture(Renderer, &Target);
	return Tex.HeapIndex;
}

UINT
R_CalculateMipLevels(INT Width, INT Height)
{
	return 1 + (uint32_t)floorf(log2f((float)max(Width, Height)));
}

void
R_GenerateMips(R_Model *Model, M_Arena *UploadArena)
{
	for (size_t i = 0; i < Model->ImagesCount; ++i) {
		R_Texture *Texture = &Model->Images[i];

		if (Texture->MipPixels[0] == NULL || Texture->MipLevels <= 1) {
			continue;
		}

		INT CurrentWidth = Texture->Width;
		INT CurrentHeight = Texture->Height;

		for (size_t MipLevel = 1; MipLevel < Texture->MipLevels; MipLevel++) {
			INT NextWidth = CurrentWidth > 1 ? CurrentWidth / 2 : 1;
			INT NextHeight = CurrentHeight > 1 ? CurrentHeight / 2 : 1;

			Texture->MipPixels[MipLevel] = (UINT8*)M_ArenaAlloc(UploadArena, NextWidth * NextHeight * 4);

			stbir_resize_uint8_linear((unsigned char *)Texture->MipPixels[MipLevel - 1], CurrentWidth, CurrentHeight, 0,
									  (unsigned char *)Texture->MipPixels[MipLevel], NextWidth, NextHeight, 0, STBIR_RGBA);

			CurrentWidth = NextWidth;
			CurrentHeight = NextHeight;
		}
	}
}