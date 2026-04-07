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

using Sendai::Renderer;

constexpr FLOAT CLEAR_COLOR[] = {0.0f, 0.0f, 0.0f, 1.0f};

Renderer::Renderer(HWND hWnd, UINT Width, UINT Height)
{
	hWnd = hWnd;
	AspectRatio = static_cast<FLOAT>(Width) / (Height);
	Viewport = {.TopLeftX = 0.0f,
				.TopLeftY = 0.0f,
				.Width = static_cast<FLOAT>(Width),
				.Height = static_cast<FLOAT>(Height),
				.MinDepth = 0.0f,
				.MaxDepth = 1.0f};
	ScissorRect = {0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)};
	State = ERS_GLTF;
	bDrawGrid = TRUE;
	TexturesCount = 0;

	MeshDataOffset = 0;
	SceneDataOffset = 0;
	CurrentUploadBufferOffset = 0;
	CurrentVertexBufferOffset = 0;
	CurrentIndexBufferOffset = 0;

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

	GetAdapter(Factory.Get());

	hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(Device.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {
	  .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	  .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
	  .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	  .NodeMask = 0,
	};
	hr = Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(CommandQueue.GetAddressOf()));
	ExitIfFailed(hr);

	FenceValue = 0;
	hr = Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.GetAddressOf()));
	ExitIfFailed(hr);

	hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandAllocator.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {
	  .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	  .NumDescriptors = PBR_N_TEXTURES_DESCRIPTORS,
	  .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	  .NodeMask = 0,
	};
	hr = Device->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(TexturesHeap.GetAddressOf()));
	ExitIfFailed(hr);
	TexturesHeap->SetName(L"TexturesHeap");

	hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(), PipelineState[State].Get(),
								   IID_PPV_ARGS(CommandList.GetAddressOf()));
	ExitIfFailed(hr);

	FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (FenceEvent == NULL) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		ExitIfFailed(hr);
	}

	D3D12_DESCRIPTOR_HEAP_DESC RtvDescHeapDesc = {
	  .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	  .NumDescriptors = FRAME_COUNT,
	  .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	  .NodeMask = 0,
	};
	hr = Device->CreateDescriptorHeap(&RtvDescHeapDesc, IID_PPV_ARGS(RtvDescriptorHeap.GetAddressOf()));
	ExitIfFailed(hr);
	RtvDescriptorHeap->SetName(L"RtvDescriptorHeap");

	for (size_t Type = 0; Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++Type) {
		DescriptorHandleIncrementSize[Type] = Device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(Type));
	}

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {
	  .Width = Width,
	  .Height = Height,
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
	hr = Factory->CreateSwapChainForHwnd((IUnknown *)CommandQueue.Get(), hWnd, &SwapChainDesc, NULL, NULL, &SwapChain);
	ExitIfFailed(hr);

	SetRtvBuffers();
	CreateSceneResources();
	CreateDepthStencilBuffer(Width, Height);
	CreateShaders();
}

VOID
Renderer::Draw(const Sendai::Scene &Scene, const Sendai::Camera &Camera)
{
	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	D3D12_RESOURCE_BARRIER ResourceBarrier = {
	  .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	  .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	  .Transition =
		  {
			.pResource = RtvBuffers[RtvIndex].Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		  },
	};

	CommandList->ResourceBarrier(1, &ResourceBarrier);
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilCPUHandle = DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
	CommandList->OMSetRenderTargets(1, &RtvHandles[RtvIndex], FALSE, &DepthStencilCPUHandle);
	CommandList->ClearRenderTargetView(RtvHandles[RtvIndex], CLEAR_COLOR, 0, NULL);
	CommandList->ClearDepthStencilView(DepthStencilCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap *Heaps[] = {TexturesHeap.Get()};
	CommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);

	UINT64 StartMeshDataOffset = MeshDataOffset;
	UINT64 StartSceneDataOffset = SceneDataOffset;

	MeshConstants MeshConstants = {.MVP = {
									 .View = Camera.ViewMatrix(),
									 .Proj = Camera.ProjectionMatrix(XM_PIDIV4, AspectRatio, 0.1f, 1000.0f),
								   }};
	RenderPrimitives(Scene, MeshConstants);
	if (bDrawGrid) {
		R_RenderGrid(this, &MeshConstants);
	}
	RenderLightBillboards(Scene.Data.Lights, Scene.ActiveLightMask, &MeshConstants);

	MeshDataOffset = StartMeshDataOffset;
	SceneDataOffset = StartSceneDataOffset;

	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarrier.Transition.pResource = RtvBuffers[RtvIndex].Get();
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	CommandList->ResourceBarrier(1, &ResourceBarrier);

	ExecuteCommands();

	HRESULT hr = SwapChain->Present(1, 0);
	ExitIfFailed(hr);
	RtvIndex = (RtvIndex + 1) % FRAME_COUNT;
}

VOID
Renderer::ExecuteCommands()
{
	CommandList->Close();
	ID3D12CommandList *CmdLists[] = {(ID3D12CommandList *)CommandList.Get()};
	CommandQueue->ExecuteCommandLists(1, CmdLists);
	SignalAndWait();
	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), PipelineState[State].Get());
}

VOID
Renderer::SwapchainResize(INT Width, INT Height)
{
	for (INT i = 0; i < FRAME_COUNT; ++i) {
		SignalAndWait();
		RtvBuffers[i].Reset();
	}

	HRESULT hr = SwapChain->ResizeBuffers(2, Width, Height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	ExitIfFailed(hr);
	hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(RtvBuffers[0].GetAddressOf()));
	ExitIfFailed(hr);
	hr = SwapChain->GetBuffer(1, IID_PPV_ARGS(RtvBuffers[1].GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	Device->CreateRenderTargetView(RtvBuffers[0].Get(), NULL, DescriptorHandle);
	RtvHandles[0] = DescriptorHandle;
	DescriptorHandle.ptr += DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	Device->CreateRenderTargetView(RtvBuffers[1].Get(), NULL, DescriptorHandle);
	RtvHandles[1] = DescriptorHandle;
	RtvIndex = 0;

	Width = Width;
	Height = Height;
	AspectRatio = (FLOAT)Width / (FLOAT)Height;
	Viewport = {.TopLeftX = 0.0f, .TopLeftY = 0.0f, .Width = (FLOAT)Width, .Height = (FLOAT)Height, .MinDepth = 0.0f, .MaxDepth = 1.0f};
	ScissorRect = {0, 0, (LONG)Width, (LONG)Height};

	CreateDepthStencilBuffer(Width, Height);
}

Renderer::~Renderer()
{
	for (INT i = 0; i < FRAME_COUNT; ++i) {
		SignalAndWait();
	}
	CloseHandle(FenceEvent);
}

VOID
Renderer::RenderLightBillboards(const Light *Lights, BYTE ActiveLightMask, MeshConstants *const MeshConstants)
{
	CommandList->SetGraphicsRootSignature(RootSignBillboard.Get());
	CommandList->SetPipelineState(PipelineState[ERS_BILLBOARD].Get());
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D12_GPU_DESCRIPTOR_HANDLE LampTextureHandle = TexturesHeap->GetGPUDescriptorHandleForHeapStart();
	LampTextureHandle.ptr += (UINT64)ERSI_BILLBOARD_LAMP * DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	CommandList->SetGraphicsRootDescriptorTable(1, LampTextureHandle);

	for (int i = 0; i < PBR_MAX_LIGHT_NUMBER; ++i) {
		if (ActiveLightMask & (1 << i)) {
			XMVECTOR LightPos = XMLoadFloat3(&Lights[i].LightPosition);
			MeshConstants->MVP.Model = XM_MAT_TRANSLATION_FROM_VEC(LightPos);
			RenderLightBillboard(MeshConstants, Lights[i].LightColor);
		}
	}
}

/****************************************************
	Implementation of private functions
*****************************************************/

VOID
Renderer::SignalAndWait()
{
	HRESULT hr = CommandQueue->Signal(Fence.Get(), ++FenceValue);
	ExitIfFailed(hr);
	Fence->SetEventOnCompletion(FenceValue, FenceEvent);
	WaitForSingleObject(FenceEvent, INFINITE);
}

VOID
Renderer::RenderPrimitives(const Scene &Scene, MeshConstants &MeshConstants)
{
	CommandList->SetGraphicsRootSignature(RootSignPBR.Get());
	UINT8 *MeshDataCpuAddress = MeshDataUploadBufferCpuAddress;

	SceneData SceneData = PreprocessSceneData(Scene);
	std::memcpy(SceneDataUploadBufferCpuAddress + SceneDataOffset, &SceneData, sizeof(SceneData));
	CommandList->SetGraphicsRootConstantBufferView(2, M_GpuAddress(SceneDataUploadBuffer.Get(), SceneDataOffset));
	SceneDataOffset += CB_ALIGN(SceneData);

	D3D12_GPU_DESCRIPTOR_HANDLE TexturesHeapStart = TexturesHeap->GetGPUDescriptorHandleForHeapStart();
	CommandList->SetGraphicsRootDescriptorTable(3, TexturesHeapStart);

	for (auto &Model : Scene.Models) {
		if (!Model.Visible) {
			continue;
		}
		XMMATRIX T = XMMatrixTranslation(Model.Position.x, Model.Position.y, Model.Position.z);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(Model.Rotation.x, Model.Rotation.y, Model.Rotation.z);
		XMMATRIX S = XMMatrixScaling(Model.Scale.x, Model.Scale.y, Model.Scale.z);
		XMMATRIX M = XM_MAT_MULT(S, R);
		M = XM_MAT_MULT(M, T);
		for (auto &Node : Model.Nodes) {
			MeshConstants.MVP.Model = XMLoadFloat4x4(&Node.ModelMatrix);
			MeshConstants.MVP.Model = XM_MAT_MULT(MeshConstants.MVP.Model, M);
			XMFLOAT4X4 ModelXMFLOAT;
			XM_STORE_FLOAT4X4(&ModelXMFLOAT, MeshConstants.MVP.Model);
			MeshConstants.Normal = R_NormalMatrix(&ModelXMFLOAT);

			std::memcpy(MeshDataCpuAddress + MeshDataOffset, &MeshConstants, sizeof(Sendai::MeshConstants));
			CommandList->SetGraphicsRootConstantBufferView(0, M_GpuAddress(MeshDataUploadBuffer.Get(), MeshDataOffset));
			MeshDataOffset += CB_ALIGN(MeshConstants);

			if (Node.MeshData) {
				Mesh *Mesh = Node.MeshData.get();
				for (INT PrimitiveIdx = 0; PrimitiveIdx < Mesh->Primitives.size(); ++PrimitiveIdx) {
					Primitive *Primitive = &Mesh->Primitives[PrimitiveIdx];
					CommandList->SetGraphicsRoot32BitConstants(1, NUM_32BITS_PBR_VALUES, &Primitive->ConstantBuffer, 0);
					CommandList->IASetVertexBuffers(0, 1, &Primitive->VertexBufferView);
					CommandList->IASetIndexBuffer(&Primitive->IndexBufferView);
					CommandList->DrawIndexedInstanced(Primitive->IndexCount, 1, 0, 0, 0);
				}
			}
		}
	}
}

VOID
Renderer::CreateDepthStencilBuffer(UINT Width, UINT Height)
{
	D3D12_DESCRIPTOR_HEAP_DESC DepthStencilHeapDesc = {
	  .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV, .NumDescriptors = 1, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE};
	HRESULT hr = Device->CreateDescriptorHeap(&DepthStencilHeapDesc, IID_PPV_ARGS(DepthStencilHeap.ReleaseAndGetAddressOf()));
	ExitIfFailed(hr);

	D3D12_RESOURCE_DESC TextureDesc = CD3DX12_TEX2D(DXGI_FORMAT_D32_FLOAT, Width, Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
													D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);
	D3D12_CLEAR_VALUE DepthOptimizedClearValue = {.Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = {.Depth = 1.0f}};
	D3D12_HEAP_PROPERTIES DefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	Device->CreateCommittedResource(&DefaultHeap, D3D12_HEAP_FLAG_NONE, &TextureDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
									&DepthOptimizedClearValue, IID_PPV_ARGS(DepthStencil.ReleaseAndGetAddressOf()));

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
	Device->CreateDepthStencilView(DepthStencil.Get(), NULL, DepthStencilHandle);

	DepthStencilHeap->SetName(L"DepthStencilHeap");
}

VOID
Renderer::SetRtvBuffers()
{
	D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptorHandle = RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FRAME_COUNT; ++i) {
		HRESULT hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(RtvBuffers[i].ReleaseAndGetAddressOf()));
		ExitIfFailed(hr);
		Device->CreateRenderTargetView(RtvBuffers[i].Get(), NULL, RtvDescriptorHandle);
		RtvHandles[i] = RtvDescriptorHandle;
		RtvDescriptorHandle.ptr += DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	}
}

VOID
Renderer::CreateSceneResources()
{
	HRESULT hr;
	D3D12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC_BUFFER(MEGABYTES(128), D3D12_RESOURCE_FLAG_NONE, 0);
	D3D12_HEAP_PROPERTIES UploadHeapProps = {.Type = D3D12_HEAP_TYPE_UPLOAD};
	D3D12_HEAP_PROPERTIES DefaultHeapProps = {.Type = D3D12_HEAP_TYPE_DEFAULT};
	hr = Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
										 IID_PPV_ARGS(UploadBuffer.GetAddressOf()));
	ExitIfFailed(hr);
	hr = Device->CreateCommittedResource(&DefaultHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_COMMON, NULL,
										 IID_PPV_ARGS(VertexBufferDefault.GetAddressOf()));
	ExitIfFailed(hr);
	hr = Device->CreateCommittedResource(&DefaultHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_COMMON, NULL,
										 IID_PPV_ARGS(IndexBufferDefault.GetAddressOf()));
	ExitIfFailed(hr);

	BufferDesc.Width = MEGABYTES(1);
	hr = Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
										 IID_PPV_ARGS(MeshDataUploadBuffer.GetAddressOf()));
	ExitIfFailed(hr);

	BufferDesc.Width = MEGABYTES(1);
	hr = Device->CreateCommittedResource(&UploadHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
										 IID_PPV_ARGS(SceneDataUploadBuffer.GetAddressOf()));
	ExitIfFailed(hr);

	D3D12_HEAP_PROPERTIES HeapProps = {.Type = D3D12_HEAP_TYPE_UPLOAD};
	TextureUploadBuffer.Size = MEGABYTES(128);
	TextureUploadBuffer.CurrentOffset = 0;
	D3D12_RESOURCE_DESC TextureBufferDesc = CD3DX12_RESOURCE_DESC_BUFFER(TextureUploadBuffer.Size, D3D12_RESOURCE_FLAG_NONE, 0);
	hr = Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &TextureBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
										 IID_PPV_ARGS(TextureUploadBuffer.Buffer.GetAddressOf()));
	ExitIfFailed(hr);
	TextureUploadBuffer.Buffer->SetName(L"Texture_Upload_Buffer");

	/* Not sure if ideal, but I decided to maintain my buffers mapped during the whole lifetime of the engine */
	D3D12_RANGE Range = {0, 0};
	TextureUploadBuffer.Buffer->Map(0, &Range, (VOID **)&TextureUploadBuffer.BaseMappedPtr);
	MeshDataUploadBuffer->Map(0, &Range, (VOID **)&MeshDataUploadBufferCpuAddress);
	SceneDataUploadBuffer->Map(0, &Range, (VOID **)&SceneDataUploadBufferCpuAddress);
	UploadBuffer->Map(0, NULL, (VOID **)&UploadBufferCpuAddress);

	CreateBaseEngineTextures();
	R_CreateGrid(this, 100.f);
}

VOID
Renderer::CreateBaseEngineTextures()
{
	auto LampImagePath = Win32FullPath(L"/assets/images/lamp.png");
	R_CreateCustomTexture(LampImagePath, this);
	std::memcpy(SceneDataUploadBufferCpuAddress + SceneDataOffset, BillboardVertices, sizeof(BillboardVertices));
	BillboardBufferLocation = M_GpuAddress(SceneDataUploadBuffer.Get(), SceneDataOffset);
	SceneDataOffset += CB_ALIGN(BillboardVertices);
}

VOID
Renderer::CreateShaders()
{
	R_CreatePBRPipelineState(this);
	R_CreateBillboardPipelineState(this);
	R_CreateGridPipelineState(this);
}

Sendai::SceneData
Renderer::PreprocessSceneData(const Sendai::Scene &Scene)
{
	SceneData Result = {0};
	Result.CameraPosition = Scene.Data.CameraPosition;
	R_UpdateLights(Scene.ActiveLightMask, Scene.Data.Lights, Result.Lights, PBR_MAX_LIGHT_NUMBER);
	return Result;
}

VOID
Renderer::GetAdapter(IDXGIFactory2 *Factory)
{
	for (UINT i = 0; SUCCEEDED(Factory->EnumAdapters(i, (IDXGIAdapter **)Adapter.GetAddressOf())); ++i) {
		DXGI_ADAPTER_DESC1 Desc = {0};
		HRESULT hr = Adapter->GetDesc1(&Desc);
		if (FAILED(hr) || Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}
		break;
	}
}

VOID
Renderer::RenderLightBillboard(const MeshConstants *const MeshConstants, XMFLOAT3 Tint)
{
	LightBillboardConstants CB = {.MVP = MeshConstants->MVP, .Tint = Tint};
	std::memcpy(MeshDataUploadBufferCpuAddress + MeshDataOffset, &CB, sizeof(LightBillboardConstants));

	CommandList->SetGraphicsRootConstantBufferView(0, M_GpuAddress(MeshDataUploadBuffer.Get(), MeshDataOffset));

	D3D12_VERTEX_BUFFER_VIEW VBV = {
	  .BufferLocation = BillboardBufferLocation,
	  .SizeInBytes = sizeof(BillboardVertices),
	  .StrideInBytes = sizeof(BillboardVertex),
	};
	CommandList->IASetVertexBuffers(0, 1, &VBV);
	CommandList->DrawInstanced(4, 1, 0, 0);

	MeshDataOffset += CB_ALIGN(LightBillboardConstants);
}