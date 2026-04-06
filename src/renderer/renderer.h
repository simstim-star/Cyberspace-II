#pragma once

#include "../assets/gltf.h"
#include "../core/scene.h"
#include "render_types.h"
#include "texture.h"
#include <string>
#include <unordered_map>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#define FRAME_COUNT 2

struct R_Camera;
struct R_Primitive;
struct R_Texture;
struct TextureLookup;

namespace Sendai {
class Camera;
}

enum EReservedSrvIndex {
	ERSI_BILLBOARD_LAMP = 0,
};

enum ERenderState { ERS_GLTF, ERS_WIREFRAME, ERS_BILLBOARD, ERS_GRID, ERS_N_RENDER_STATES };

struct R_UploadBuffer {
	ComPtr<ID3D12Resource> Buffer;
	UINT8 *BaseMappedPtr;
	UINT64 Size;
	UINT64 CurrentOffset;
};

struct R_Core {
	HWND hWnd;
	UINT Width;
	UINT Height;
	FLOAT AspectRatio;

	ComPtr<IDXGIAdapter3> Adapter;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;

	ComPtr<ID3D12Device> Device;
	ComPtr<IDXGISwapChain1> SwapChain;
	ComPtr<ID3D12DescriptorHeap> RtvDescriptorHeap;

	ComPtr<ID3D12Resource> RtvBuffers[FRAME_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE RtvHandles[FRAME_COUNT];
	UINT RtvIndex;

	ComPtr<ID3D12DescriptorHeap> TexturesHeap;
	UINT TexturesCount;
	R_UploadBuffer TextureUploadBuffer;
	std::unordered_map<std::wstring, GPUTexture> Textures;

	ComPtr<ID3D12RootSignature> RootSignPBR;
	ComPtr<ID3D12RootSignature> RootSignBillboard;
	ComPtr<ID3D12RootSignature> RootSignGrid;

	ComPtr<ID3D12Resource> DepthStencil;
	ComPtr<ID3D12DescriptorHeap> DepthStencilHeap;

	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> CommandList;

	ERenderState State;
	BOOL bDrawGrid;
	ComPtr<ID3D12PipelineState> PipelineState[ERS_N_RENDER_STATES];

	UINT DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	/*****************************
		Synchronization objects
	*****************************/

	UINT FrameIndex;
	UINT64 FenceValue;
	HANDLE FenceEvent;
	ComPtr<ID3D12Fence> Fence;

	/*****************************
		Resources
	*****************************/

	ComPtr<ID3D12Resource> VertexBufferDefault;
	ComPtr<ID3D12Resource> IndexBufferDefault;
	ComPtr<ID3D12Resource> UploadBuffer;
	UINT8 *UploadBufferCpuAddress;

	UINT8 *MeshDataUploadBufferCpuAddress;
	ComPtr<ID3D12Resource> MeshDataUploadBuffer;
	UINT64 MeshDataOffset;

	UINT8 *SceneDataUploadBufferCpuAddress;
	ComPtr<ID3D12Resource> SceneDataUploadBuffer;
	UINT64 SceneDataOffset;

	D3D12_GPU_VIRTUAL_ADDRESS BillboardBufferLocation;
	D3D12_GPU_VIRTUAL_ADDRESS GridBufferLocation;

	UINT64 CurrentUploadBufferOffset;
	UINT64 CurrentVertexBufferOffset;
	UINT64 CurrentIndexBufferOffset;
};

void R_Init(R_Core *const Renderer, HWND hWnd);
void R_Destroy(R_Core *Renderer);
void R_Draw(R_Core *const Renderer, const S_Scene *const Scene, const Sendai::Camera *const Camera);
void Draw(R_Core *const Renderer, const Sendai::Camera *const Camera, const S_Scene *const Scene);
void R_ExecuteCommands(R_Core *const Renderer);
void R_SwapchainResize(R_Core *const Renderer, INT Width, INT Height);
