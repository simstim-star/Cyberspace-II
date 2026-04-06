#include "../core/pch.h"

#include "light.h"
#include "render_types.h"
#include "renderer.h"
#include "shader.h"
#include "texture.h"

#include "../core/camera.h"
#include "../core/grid.h"
#include "../core/memory.h"
#include "../dx_helpers/desc_helpers.h"
#include "../error/error.h"
#include "../shaders/sendai/shader_defs.h"
#include "../win32/win_path.h"

#include "billboard.h"

constexpr FLOAT CLEAR_COLOR[] = {0.0f, 0.0f, 0.0f, 1.0f};

/****************************************************
	Forward declaration of private functions
*****************************************************/

static void SetRtvBuffers(R_Core *const Renderer, UINT NumBuffers);
static void CreateSceneResources(R_Core *const Renderer);
static void CreateBaseEngineTextures(R_Core *const Renderer);
static void CreateDepthStencilBuffer(R_Core *const Renderer);
static void CreateShaders(R_Core *const Renderer);
static R_SceneData PreprocessSceneData(const S_Scene *const Scene);
static void CreateBaseEngineTextures(R_Core *const Renderer);
static void GetAdapter(IDXGIFactory2 *Factory, R_Core *Renderer);

static void SignalAndWait(R_Core *const Renderer);
static void RenderPrimitives(const S_Scene *const Scene, R_Core *const Renderer, R_MeshConstants *const MeshConstants);

/****************************************************
Public functions
*****************************************************/

void
R_Init(R_Core *const Renderer, HWND hWnd)
{
	Renderer->hWnd = hWnd;
	Renderer->AspectRatio = (FLOAT)(Renderer->Width) / (Renderer->Height);
	Renderer->Viewport = {.TopLeftX = 0.0f,
						  .TopLeftY = 0.0f,
						  .Width = (FLOAT)(Renderer->Width),
						  .Height = (FLOAT)(Renderer->Height),
						  .MinDepth = 0.0f,
						  .MaxDepth = 1.0f};
	Renderer->ScissorRect = {0, 0, (LONG)(Renderer->Width), (LONG)(Renderer->Height)};
	Renderer->State = ERS_GLTF;
	Renderer->bDrawGrid = TRUE;

	/* D3D12 setup */

	INT bIsDebugFactory = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug1> DebugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(DebugController.GetAddressOf())))) {
		DebugController->EnableDebugLayer();
		// ID3D12Debug1_SetEnableGPUBasedValidation(DebugController, 1);
		bIsDebugFactory |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ComPtr<IDXGIFactory2> Factory = NULL;
	HRESULT hr = CreateDXGIFactory2(bIsDebugFactory, IID_PPV_ARGS(Factory.GetAddressOf()));
	ExitIfFailed(hr);

	GetAdapter(Factory.Get(), Renderer);

	hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(Renderer->Device.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {
	  .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	  .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
	  .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	  .NodeMask = 0,
	};
	hr = Renderer->Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(Renderer->CommandQueue.GetAddressOf()));
	ExitIfFailed(hr);

	Renderer->FenceValue = 0;
	hr = Renderer->Device->CreateFence(Renderer->FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Renderer->Fence.GetAddressOf()));
	ExitIfFailed(hr);

	hr = Renderer->Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(Renderer->CommandAllocator.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {
	  .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	  .NumDescriptors = PBR_N_TEXTURES_DESCRIPTORS,
	  .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	  .NodeMask = 0,
	};
	hr = Renderer->Device->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(Renderer->TexturesHeap.GetAddressOf()));
	ExitIfFailed(hr);
	Renderer->TexturesHeap->SetName(L"TexturesHeap");

	hr = Renderer->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Renderer->CommandAllocator.Get(),
											 Renderer->PipelineState[Renderer->State].Get(), IID_PPV_ARGS(Renderer->CommandList.GetAddressOf()));
	ExitIfFailed(hr);

	Renderer->FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (Renderer->FenceEvent == NULL) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		ExitIfFailed(hr);
	}

	D3D12_DESCRIPTOR_HEAP_DESC RtvDescHeapDesc = {
	  .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	  .NumDescriptors = FRAME_COUNT,
	  .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	  .NodeMask = 0,
	};
	hr = Renderer->Device->CreateDescriptorHeap(&RtvDescHeapDesc, IID_PPV_ARGS(Renderer->RtvDescriptorHeap.GetAddressOf()));
	ExitIfFailed(hr);
	Renderer->RtvDescriptorHeap->SetName(L"RtvDescriptorHeap");

	for (size_t Type = 0; Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++Type) {
		Renderer->DescriptorHandleIncrementSize[Type] =
			Renderer->Device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(Type));
	}

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {
	  .Width = Renderer->Width,
	  .Height = Renderer->Height,
	  .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	  .Stereo = 0,
	  .SampleDesc =
		  {
			.Count = 1,
			.Quality = 0,
		  },
	  .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	  .BufferCount = 2,
	  .Scaling = DXGI_SCALING_STRETCH,
	  .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
	  .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
	  .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
	};
	hr = Factory->CreateSwapChainForHwnd((IUnknown *)Renderer->CommandQueue.Get(), Renderer->hWnd, &SwapChainDesc, NULL, NULL,
										 &Renderer->SwapChain);
	ExitIfFailed(hr);

	SetRtvBuffers(Renderer, FRAME_COUNT);
	CreateSceneResources(Renderer);
	CreateDepthStencilBuffer(Renderer);
	CreateShaders(Renderer);
}

void
R_Draw(R_Core *const Renderer, const S_Scene *const Scene, const Sendai::Camera *const Camera)
{
	Renderer->CommandList->RSSetViewports(1, &Renderer->Viewport);
	Renderer->CommandList->RSSetScissorRects(1, &Renderer->ScissorRect);

	D3D12_RESOURCE_BARRIER ResourceBarrier = {
	  .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	  .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	  .Transition =
		  {
			.pResource = Renderer->RtvBuffers[Renderer->RtvIndex].Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		  },
	};

	Renderer->CommandList->ResourceBarrier(1, &ResourceBarrier);
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilCPUHandle = Renderer->DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
	Renderer->CommandList->OMSetRenderTargets(1, &Renderer->RtvHandles[Renderer->RtvIndex], FALSE, &DepthStencilCPUHandle);
	Renderer->CommandList->ClearRenderTargetView(Renderer->RtvHandles[Renderer->RtvIndex], CLEAR_COLOR, 0, NULL);
	Renderer->CommandList->ClearDepthStencilView(DepthStencilCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	Renderer->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap *Heaps[] = {Renderer->TexturesHeap.Get()};
	Renderer->CommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);

	Draw(Renderer, Camera, Scene);

	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarrier.Transition.pResource = Renderer->RtvBuffers[Renderer->RtvIndex].Get();
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Renderer->CommandList->ResourceBarrier(1, &ResourceBarrier);

	R_ExecuteCommands(Renderer);

	HRESULT hr = Renderer->SwapChain->Present(1, 0);
	ExitIfFailed(hr);
	Renderer->RtvIndex = (Renderer->RtvIndex + 1) % FRAME_COUNT;
}

void
R_ExecuteCommands(R_Core *const Renderer)
{
	Renderer->CommandList->Close();
	ID3D12CommandList *CmdLists[] = {(ID3D12CommandList *)Renderer->CommandList.Get()};
	Renderer->CommandQueue->ExecuteCommandLists(1, CmdLists);
	SignalAndWait(Renderer);
	Renderer->CommandAllocator->Reset();
	Renderer->CommandList->Reset(Renderer->CommandAllocator.Get(), Renderer->PipelineState[Renderer->State].Get());
}

void
R_SwapchainResize(R_Core *const Renderer, INT Width, INT Height)
{
	for (INT i = 0; i < FRAME_COUNT; ++i) {
		SignalAndWait(Renderer);
		Renderer->RtvBuffers[i].Reset();
	}

	HRESULT hr = Renderer->SwapChain->ResizeBuffers(2, Width, Height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	ExitIfFailed(hr);
	hr = Renderer->SwapChain->GetBuffer(0, IID_PPV_ARGS(Renderer->RtvBuffers[0].GetAddressOf()));
	ExitIfFailed(hr);
	hr = Renderer->SwapChain->GetBuffer(1, IID_PPV_ARGS(Renderer->RtvBuffers[1].GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = Renderer->RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	Renderer->Device->CreateRenderTargetView(Renderer->RtvBuffers[0].Get(), NULL, DescriptorHandle);
	Renderer->RtvHandles[0] = DescriptorHandle;
	DescriptorHandle.ptr += Renderer->DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	Renderer->Device->CreateRenderTargetView(Renderer->RtvBuffers[1].Get(), NULL, DescriptorHandle);
	Renderer->RtvHandles[1] = DescriptorHandle;
	Renderer->RtvIndex = 0;

	Renderer->Width = Width;
	Renderer->Height = Height;
	Renderer->AspectRatio = (FLOAT)Width / (FLOAT)Height;
	Renderer->Viewport = {
	  .TopLeftX = 0.0f, .TopLeftY = 0.0f, .Width = (FLOAT)Width, .Height = (FLOAT)Height, .MinDepth = 0.0f, .MaxDepth = 1.0f};
	Renderer->ScissorRect = {0, 0, (LONG)Width, (LONG)Height};

	CreateDepthStencilBuffer(Renderer);
}

void
R_Destroy(R_Core *Renderer)
{
	for (INT i = 0; i < FRAME_COUNT; ++i) {
		SignalAndWait(Renderer);
	}
	CloseHandle(Renderer->FenceEvent);
}

/****************************************************
	Implementation of private functions
*****************************************************/

static void
SignalAndWait(R_Core *const Renderer)
{
	HRESULT hr = Renderer->CommandQueue->Signal(Renderer->Fence.Get(), ++Renderer->FenceValue);
	ExitIfFailed(hr);
	Renderer->Fence->SetEventOnCompletion(Renderer->FenceValue, Renderer->FenceEvent);
	WaitForSingleObject(Renderer->FenceEvent, INFINITE);
}

void
RenderPrimitives(const S_Scene *const Scene, R_Core *const Renderer, R_MeshConstants *const MeshConstants)
{
	Renderer->CommandList->SetGraphicsRootSignature(Renderer->RootSignPBR.Get());
	UINT8 *MeshDataCpuAddress = Renderer->MeshDataUploadBufferCpuAddress;

	R_SceneData SceneData = PreprocessSceneData(Scene);
	memcpy(Renderer->SceneDataUploadBufferCpuAddress + Renderer->SceneDataOffset, &SceneData, sizeof(R_SceneData));
	Renderer->CommandList->SetGraphicsRootConstantBufferView(2, M_GpuAddress(Renderer->SceneDataUploadBuffer.Get(), Renderer->SceneDataOffset));
	Renderer->SceneDataOffset += CB_ALIGN(R_SceneData);

	D3D12_GPU_DESCRIPTOR_HANDLE TexturesHeapStart = Renderer->TexturesHeap->GetGPUDescriptorHandleForHeapStart();
	Renderer->CommandList->SetGraphicsRootDescriptorTable(3, TexturesHeapStart);

	for (size_t ModelIndex = 0; ModelIndex < Scene->ModelsCount; ++ModelIndex) {
		R_Model *Model = &Scene->Models[ModelIndex];
		if (!Model->Visible) {
			continue;
		}
		XMMATRIX T = XMMatrixTranslation(Model->Position.x, Model->Position.y, Model->Position.z);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(Model->Rotation.x, Model->Rotation.y, Model->Rotation.z);
		XMMATRIX S = XMMatrixScaling(Model->Scale.x, Model->Scale.y, Model->Scale.z);
		XMMATRIX M = XM_MAT_MULT(S, R);
		M = XM_MAT_MULT(M, T);
		for (size_t NodeIndex = 0; NodeIndex < Model->NodesCount; ++NodeIndex) {
			R_Node *Node = &Model->Nodes[NodeIndex];

			MeshConstants->MVP.Model = XMLoadFloat4x4(&Node->ModelMatrix);
			MeshConstants->MVP.Model = XM_MAT_MULT(MeshConstants->MVP.Model, M);
			XMFLOAT4X4 ModelXMFLOAT;
			XM_STORE_FLOAT4X4(&ModelXMFLOAT, MeshConstants->MVP.Model);
			MeshConstants->Normal = R_NormalMatrix(&ModelXMFLOAT);

			memcpy(MeshDataCpuAddress + Renderer->MeshDataOffset, MeshConstants, sizeof(R_MeshConstants));
			Renderer->CommandList->SetGraphicsRootConstantBufferView(
				0, M_GpuAddress(Renderer->MeshDataUploadBuffer.Get(), Renderer->MeshDataOffset));
			Renderer->MeshDataOffset += CB_ALIGN(R_MeshConstants);

			if (Node->Mesh != NULL) {
				R_Mesh *Mesh = Node->Mesh;
				for (INT PrimitiveIdx = 0; PrimitiveIdx < Mesh->PrimitivesCount; ++PrimitiveIdx) {
					R_Primitive *Primitive = &Mesh->Primitives[PrimitiveIdx];
					Renderer->CommandList->SetGraphicsRoot32BitConstants(1, NUM_32BITS_PBR_VALUES, &Primitive->ConstantBuffer, 0);
					Renderer->CommandList->IASetVertexBuffers(0, 1, &Primitive->VertexBufferView);
					Renderer->CommandList->IASetIndexBuffer(&Primitive->IndexBufferView);
					Renderer->CommandList->DrawIndexedInstanced(Primitive->IndexCount, 1, 0, 0, 0);
				}
			}
		}
	}
}

void
CreateDepthStencilBuffer(R_Core *const Renderer)
{
	D3D12_DESCRIPTOR_HEAP_DESC DepthStencilHeapDesc = {
	  .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE};
	HRESULT hr =
		Renderer->Device->CreateDescriptorHeap(&DepthStencilHeapDesc, IID_PPV_ARGS(Renderer->DepthStencilHeap.ReleaseAndGetAddressOf()));
	ExitIfFailed(hr);

	D3D12_RESOURCE_DESC TextureDesc = CD3DX12_TEX2D(DXGI_FORMAT_D32_FLOAT, Renderer->Width, Renderer->Height, 1, 0, 1, 0,
													D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
	D3D12_CLEAR_VALUE DepthOptimizedClearValue = {.Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = {.Depth = 1.0f}};
	D3D12_HEAP_PROPERTIES DefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	Renderer->Device->CreateCommittedResource(&DefaultHeap, D3D12_HEAP_FLAG_NONE, &TextureDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
											  &DepthOptimizedClearValue, IID_PPV_ARGS(Renderer->DepthStencil.ReleaseAndGetAddressOf()));

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = Renderer->DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
	Renderer->Device->CreateDepthStencilView(Renderer->DepthStencil.Get(), NULL, DepthStencilHandle);

	Renderer->DepthStencilHeap->SetName(L"DepthStencilHeap");
}

void
SetRtvBuffers(R_Core *const Renderer, UINT NumBuffers)
{
	D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptorHandle = Renderer->RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < NumBuffers; ++i) {
		HRESULT hr = Renderer->SwapChain->GetBuffer(i, IID_PPV_ARGS(Renderer->RtvBuffers[i].ReleaseAndGetAddressOf()));
		ExitIfFailed(hr);
		Renderer->Device->CreateRenderTargetView(Renderer->RtvBuffers[i].Get(), NULL, RtvDescriptorHandle);
		Renderer->RtvHandles[i] = RtvDescriptorHandle;
		RtvDescriptorHandle.ptr += Renderer->DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	}
}

void
CreateSceneResources(R_Core *const Renderer)
{
	HRESULT hr;
	D3D12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC_BUFFER(MEGABYTES(128), D3D12_RESOURCE_FLAG_NONE, 0);
	D3D12_HEAP_PROPERTIES UploadHeapProps = {.Type = D3D12_HEAP_TYPE_UPLOAD};
	D3D12_HEAP_PROPERTIES DefaultHeapProps = {.Type = D3D12_HEAP_TYPE_DEFAULT};
	hr = Renderer->Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
												   IID_PPV_ARGS(Renderer->UploadBuffer.GetAddressOf()));
	ExitIfFailed(hr);
	hr = Renderer->Device->CreateCommittedResource(&DefaultHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_COMMON, NULL,
												   IID_PPV_ARGS(Renderer->VertexBufferDefault.GetAddressOf()));
	ExitIfFailed(hr);
	hr = Renderer->Device->CreateCommittedResource(&DefaultHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_COMMON, NULL,
												   IID_PPV_ARGS(Renderer->IndexBufferDefault.GetAddressOf()));
	ExitIfFailed(hr);

	BufferDesc.Width = MEGABYTES(1);
	hr = Renderer->Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
												   IID_PPV_ARGS(Renderer->MeshDataUploadBuffer.GetAddressOf()));
	ExitIfFailed(hr);

	BufferDesc.Width = MEGABYTES(1);
	hr = Renderer->Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
												   IID_PPV_ARGS(Renderer->SceneDataUploadBuffer.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_HEAP_PROPERTIES HeapProps = {.Type = D3D12_HEAP_TYPE_UPLOAD};
	Renderer->TextureUploadBuffer.Size = MEGABYTES(128);
	Renderer->TextureUploadBuffer.CurrentOffset = 0;
	D3D12_RESOURCE_DESC TextureBufferDesc = CD3DX12_RESOURCE_DESC_BUFFER(Renderer->TextureUploadBuffer.Size, D3D12_RESOURCE_FLAG_NONE, 0);
	hr = Renderer->Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TextureBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
												   IID_PPV_ARGS(Renderer->TextureUploadBuffer.Buffer.GetAddressOf()));
	ExitIfFailed(hr);
	Renderer->TextureUploadBuffer.Buffer->SetName(L"Texture_Upload_Buffer");

	/* Not sure if ideal, but I decided to maintain my buffers mapped during the whole lifetime of the engine */
	D3D12_RANGE Range = {0, 0};
	Renderer->TextureUploadBuffer.Buffer->Map(0, &Range, (void **)&Renderer->TextureUploadBuffer.BaseMappedPtr);
	Renderer->MeshDataUploadBuffer->Map(0, &Range, (void **)&Renderer->MeshDataUploadBufferCpuAddress);
	Renderer->SceneDataUploadBuffer->Map(0, &Range, (void **)&Renderer->SceneDataUploadBufferCpuAddress);
	Renderer->UploadBuffer->Map(0, NULL, (void **)&Renderer->UploadBufferCpuAddress);

	CreateBaseEngineTextures(Renderer);
	R_CreateGrid(Renderer, 100.f);
}

void
CreateBaseEngineTextures(R_Core *const Renderer)
{
	auto LampImagePath = Win32FullPath(L"/assets/images/lamp.png");
	R_CreateCustomTexture(LampImagePath, Renderer);
	memcpy(Renderer->SceneDataUploadBufferCpuAddress + Renderer->SceneDataOffset, BillboardVertices, sizeof(BillboardVertices));
	Renderer->BillboardBufferLocation = M_GpuAddress(Renderer->SceneDataUploadBuffer.Get(), Renderer->SceneDataOffset);
	Renderer->SceneDataOffset += CB_ALIGN(BillboardVertices);
}

void
CreateShaders(R_Core *const Renderer)
{
	R_CreatePBRPipelineState(Renderer);
	R_CreateBillboardPipelineState(Renderer);
	R_CreateGridPipelineState(Renderer);
}

R_SceneData
PreprocessSceneData(const S_Scene *const Scene)
{
	R_SceneData Result = {0};
	Result.CameraPosition = Scene->Data.CameraPosition;
	R_UpdateLights(Scene->ActiveLightMask, Scene->Data.Lights, Result.Lights, PBR_MAX_LIGHT_NUMBER);
	return Result;
}

void
Draw(R_Core *const Renderer, const Sendai::Camera *const Camera, const S_Scene *const Scene)
{
	UINT64 StartMeshDataOffset = Renderer->MeshDataOffset;
	UINT64 StartSceneDataOffset = Renderer->SceneDataOffset;
	R_MeshConstants MeshConstants = {.MVP = {
									   .View = Camera->ViewMatrix(),
									   .Proj = Camera->ProjectionMatrix(XM_PIDIV4, Renderer->AspectRatio, 0.1f, 1000.0f),
									 }};
	RenderPrimitives(Scene, Renderer, &MeshConstants);
	if (Renderer->bDrawGrid) {
		R_RenderGrid(Renderer, &MeshConstants);
	}
	R_RenderLightBillboards(Renderer, Scene->Data.Lights, Scene->ActiveLightMask, &MeshConstants);
	// UI_Draw(Renderer->CommandList);

	Renderer->MeshDataOffset = StartMeshDataOffset;
	Renderer->SceneDataOffset = StartSceneDataOffset;
}

void
GetAdapter(IDXGIFactory2 *Factory, R_Core *Renderer)
{
	for (UINT i = 0; SUCCEEDED(Factory->EnumAdapters(i, (IDXGIAdapter **)Renderer->Adapter.GetAddressOf())); ++i) {
		DXGI_ADAPTER_DESC1 Desc = {0};
		HRESULT hr = Renderer->Adapter->GetDesc1(&Desc);
		if (FAILED(hr) || Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}
		break;
	}
}