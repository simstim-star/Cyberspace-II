#include "../core/pch.h"

#include "billboard.h"
#include "light.h"

#include "../core/camera.h"
#include "../core/engine.h"
#include "../core/memory.h"
#include "../core/scene.h"
#include "../renderer/renderer.h"
#include "../shaders/sendai/shader_defs.h"

using namespace DirectX;

static VOID RenderLightBillboard(const Sendai::MeshConstants &MeshConstants, Sendai::Renderer &Renderer, XMFLOAT3 Tint);

VOID Sendai::LightsInit(Sendai::Scene &Scene, const Sendai::Camera &Camera)
{
    Scene.ActiveLightMask = 0;
    Scene.ActiveLightMask |= (1 << 0);
    Scene.Data = {
        .Lights =
            {
                {.LightPosition = {0.0f, 10.0f, 0.0f}, .LightColor = {300.0f, 100.0f, 100.0f}},
                {.LightPosition = {0.0f, 0.0f, 0.0f}, .LightColor = {100.0f, 100.0f, 100.0f}},
                {.LightPosition = {0.0f, 0.0f, 0.0f}, .LightColor = {100.0f, 100.0f, 100.0f}},
                {.LightPosition = {0.0f, 0.0f, 0.0f}, .LightColor = {100.0f, 100.0f, 100.0f}},
                {.LightPosition = {0.0f, 0.0f, 0.0f}, .LightColor = {100.0f, 100.0f, 100.0f}},
                {.LightPosition = {0.0f, 0.0f, 0.0f}, .LightColor = {100.0f, 100.0f, 100.0f}},
                {.LightPosition = {0.0f, 0.0f, 0.0f}, .LightColor = {100.0f, 100.0f, 100.0f}},
            },
        .CameraPosition = Camera.GetPosition(),
    };
}

VOID Sendai::UpdateLights(BYTE ActiveLightMask, const Sendai::Light *const InLights, Sendai::Light *const OutLights,
                          UINT NumLights)
{
    for (UINT i = 0; i < NumLights; i++)
    {
        if (IS_LIGHT_ACTIVE(ActiveLightMask, i))
        {
            OutLights[i].LightColor = InLights[i].LightColor;
            OutLights[i].LightPosition = InLights[i].LightPosition;
        }
    }
}

VOID Sendai::RenderLightBillboards(Sendai::Renderer &Renderer, const Sendai::Light *Lights, BYTE ActiveLightMask,
                                   Sendai::MeshConstants &MeshConstants)
{
    Renderer.CommandList->SetGraphicsRootSignature(Renderer.RootSignBillboard.Get());
    Renderer.CommandList->SetPipelineState(Renderer.PipelineState[Sendai::ERenderState::ERS_BILLBOARD].Get());
    Renderer.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    D3D12_GPU_DESCRIPTOR_HANDLE LampTextureHandle = Renderer.Textures->GetHeapDescriptorHandle();
    LampTextureHandle.ptr += (UINT64)Sendai::EReservedSrvIndex::ERSI_BILLBOARD_LAMP *
                             Renderer.DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    Renderer.CommandList->SetGraphicsRootDescriptorTable(1, LampTextureHandle);

    for (int i = 0; i < PBR_MAX_LIGHT_NUMBER; ++i)
    {
        if (ActiveLightMask & (1 << i))
        {
            XMVECTOR LightPos = XMLoadFloat3(&Lights[i].LightPosition);
            MeshConstants.MVP.Model = XMMatrixTranslationFromVector(LightPos);
            RenderLightBillboard(MeshConstants, Renderer, Lights[i].LightColor);
        }
    }
}

VOID RenderLightBillboard(const Sendai::MeshConstants &MeshConstants, Sendai::Renderer &Renderer, XMFLOAT3 Tint)
{
    Sendai::LightBillboardConstants CB = {.MVP = MeshConstants.MVP, .Tint = Tint};

    const auto Address = Renderer.MeshDataUploadBuffer.PushBack(CB);

    Renderer.CommandList->SetGraphicsRootConstantBufferView(0, Address);

    D3D12_VERTEX_BUFFER_VIEW VBV = {
        .BufferLocation = Renderer.BillboardBufferLocation,
        .SizeInBytes = sizeof(BillboardVertices),
        .StrideInBytes = sizeof(BillboardVertex),
    };
    Renderer.CommandList->IASetVertexBuffers(0, 1, &VBV);
    Renderer.CommandList->DrawInstanced(4, 1, 0, 0);
}