#pragma once

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

struct Sendai::Renderer;
struct Sendai::Texture;

struct GPUTexture {
	ComPtr<ID3D12Resource> GpuTexture;
	UINT HeapIndex;
};

void R_CreateCustomTexture(std::wstring &Path, Sendai::Renderer *Renderer);
void R_CreateUITexture(std::wstring &Path, Sendai::Renderer *Renderer, UINT nkSlotIndex);
GPUTexture R_UploadTexture(Sendai::Renderer *const Renderer, const Sendai::Texture *const Source);
UINT R_CalculateMipLevels(INT Width, INT Height);
ComPtr<ID3D12Resource> R_CommandCreateTextureGPU(Sendai::Renderer *const Renderer, const Sendai::Texture *const SourceTexture);
UINT64 R_SuballocateTextureUpload(Sendai::Renderer *Renderer, UINT64 Size);
UINT32
R_GetTextureIndex(Sendai::Renderer *const Renderer, const Sendai::Texture *const Texture);
void R_GenerateMips(Sendai::Model *Model, M_Arena *UploadArena);
