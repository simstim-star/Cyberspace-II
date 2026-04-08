#pragma once

#include "../assets/gltf.h"
#include "../core/scene.h"
#include "../core/upload_buffer.h"
#include "render_types.h"
#include "texture.h"

#include <string>
#include <unordered_map>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

constexpr UINT FRAME_COUNT = 2;

struct R_Primitive;
struct Sendai::Texture;
struct TextureLookup;

namespace Sendai
{
class Camera;

enum EReservedSrvIndex
{
    ERSI_BILLBOARD_LAMP = 0,
};

enum ERenderState
{
    ERS_GLTF,
    ERS_WIREFRAME,
    ERS_BILLBOARD,
    ERS_GRID,
    ERS_N_RENDER_STATES
};

struct Renderer
{
    FLOAT AspectRatio;
    ComPtr<IDXGIAdapter3> Adapter;

    D3D12_VIEWPORT Viewport;
    D3D12_RECT ScissorRect;

    ComPtr<ID3D12Device> Device;
    ComPtr<IDXGISwapChain1> SwapChain;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<ID3D12CommandAllocator> CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> CommandList;

    ComPtr<ID3D12DescriptorHeap> RtvDescriptorHeap;
    ComPtr<ID3D12Resource> RtvBuffers[FRAME_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandles[FRAME_COUNT];
    UINT RtvIndex;

    std::unique_ptr<Textures> Textures;

    ComPtr<ID3D12RootSignature> RootSignPBR;
    ComPtr<ID3D12RootSignature> RootSignBillboard;
    ComPtr<ID3D12RootSignature> RootSignGrid;

    ComPtr<ID3D12Resource> DepthStencil;
    ComPtr<ID3D12DescriptorHeap> DepthStencilHeap;

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

    Renderer(HWND hWnd, UINT Width, UINT Height);
    ~Renderer();
    VOID Draw(const Sendai::Scene &Scene, const Sendai::Camera &Camera);
    VOID ExecuteCommands();
    VOID SwapchainResize(INT Width, INT Height);
    VOID RenderLightBillboards(const Light *Lights, BYTE ActiveLightMask, MeshConstants *const MeshConstants);

  private:
    VOID SetRtvBuffers();
    VOID GetAdapter(IDXGIFactory2 *Factory);

    VOID CreateSceneResources();
    VOID CreateBaseEngineTextures();
    VOID CreateDepthStencilBuffer(UINT Width, UINT Height);
    VOID CreateShaders();

    SceneData PreprocessSceneData(const Scene &Scene);
    VOID SignalAndWait();
    VOID RenderPrimitives(const Scene &Scene, MeshConstants &MeshConstants);
    VOID RenderLightBillboard(const MeshConstants *const MeshConstants, XMFLOAT3 Tint);
};

} // namespace Sendai