#pragma once

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

struct R_Core;
struct S_Scene;
struct R_Texture;
struct R_Primitive;

struct GPUTexture {
	ComPtr<ID3D12Resource> GpuTexture;
	UINT HeapIndex;
};

void R_CreateCustomTexture(std::wstring &Path, R_Core *Renderer);
void R_CreateUITexture(std::wstring &Path, R_Core *Renderer, UINT nkSlotIndex);
GPUTexture R_UploadTexture(R_Core *const Renderer, const R_Texture *const Source);
UINT R_CalculateMipLevels(INT Width, INT Height);
ComPtr<ID3D12Resource> R_CommandCreateTextureGPU(R_Core *const Renderer, const R_Texture *const SourceTexture);
UINT64 R_SuballocateTextureUpload(R_Core *Renderer, UINT64 Size);
UINT32
R_GetTextureIndex(R_Core *const Renderer, const R_Texture *const Texture);
void R_GenerateMips(R_Model *Model, M_Arena *UploadArena);
