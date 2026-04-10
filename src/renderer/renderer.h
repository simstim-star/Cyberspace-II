#pragma once

#include "../core/upload_buffer.h"
#include "render_types.h"
#include "../core/scene.h"
#include "../core/camera.h"

constexpr UINT FRAME_COUNT = 2;

namespace Sendai
{
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
    Microsoft::WRL::ComPtr<IDXGIAdapter3> Adapter;

    D3D12_VIEWPORT Viewport;
    D3D12_RECT ScissorRect;

    Microsoft::WRL::ComPtr<ID3D12Device> Device;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> RtvBuffers[FRAME_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandles[FRAME_COUNT];
    UINT RtvIndex;

    std::unique_ptr<Textures> Textures;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignPBR;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignBillboard;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignGrid;

    Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencil;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthStencilHeap;

    ERenderState State;
    BOOL bDrawGrid;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState[ERS_N_RENDER_STATES];

    UINT DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    /*****************************
        Synchronization objects
    *****************************/

    UINT FrameIndex;
    UINT64 FenceValue;
    HANDLE FenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence> Fence;

    /*****************************
        Resources
    *****************************/

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferDefault;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferDefault;

    Sendai::UploadBuffer GeometryUploadBuffer;
    Sendai::UploadBuffer MeshDataUploadBuffer;
    Sendai::UploadBuffer SceneDataUploadBuffer;

    D3D12_GPU_VIRTUAL_ADDRESS BillboardBufferLocation;
    D3D12_GPU_VIRTUAL_ADDRESS GridBufferLocation;

    UINT64 CurrentVertexBufferOffset;
    UINT64 CurrentIndexBufferOffset;

    Renderer(HWND hWnd, UINT Width, UINT Height);
    ~Renderer();
    VOID BeginDraw(Sendai::Scene &Scene, const Sendai::Camera &Camera);
    VOID EndDraw();
    VOID ExecuteCommands();
    VOID SwapchainResize(INT Width, INT Height);
    VOID RenderLightBillboards(const Light *Lights, BYTE ActiveLightMask, MeshConstants *const MeshConstants);

  private:
    VOID _SetRtvBuffers();
    VOID _GetAdapter(IDXGIFactory2 *Factory);

    VOID _CreateSceneResources();
    VOID _CreateBaseEngineTextures();
    VOID _CreateDepthStencilBuffer(UINT Width, UINT Height);
    VOID _CreateShaders();

    SceneData _PreprocessSceneData(const Scene &Scene);
    VOID _SignalAndWait();
    VOID _RenderPrimitives(const Scene &Scene, MeshConstants &MeshConstants);
    VOID _RenderLightBillboard(const MeshConstants *const MeshConstants, DirectX::XMFLOAT3 Tint);
    VOID _RenderNode(const Sendai::Model &Model, DirectX::XMMATRIX ParentTransform,
                    Sendai::MeshConstants &MeshConstants);
};

} // namespace Sendai