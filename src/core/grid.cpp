#include "pch.h"


#include "grid.h"
#include "../renderer/renderer.h"
#include "../renderer/render_types.h"

#include <DirectXMath.h>

using namespace DirectX;

#define GRID_VERTICES_COUNT 300
static XMFLOAT3 GRID_VERTICES[GRID_VERTICES_COUNT] = {};

VOID Sendai::CreateGrid(Sendai::Renderer &Renderer, const FLOAT HalfSide)
{
    const INT LinesPerDirection = (GRID_VERTICES_COUNT / 4);
    const FLOAT Step = (HalfSide * 2.0f) / (FLOAT)(LinesPerDirection - 1);

    for (INT Line = 0; Line < LinesPerDirection; Line++)
    {
        FLOAT Position = -HalfSide + (Line * Step);
        INT i = Line * 4;
        GRID_VERTICES[i] = {Position, 0.0f, -HalfSide};
        GRID_VERTICES[i + 1] = {Position, 0.0f, HalfSide};
        GRID_VERTICES[i + 2] = {-HalfSide, 0.0f, Position};
        GRID_VERTICES[i + 3] = {HalfSide, 0.0f, Position};
    }

    Renderer.GridBufferLocation = Renderer.SceneDataUploadBuffer.Insert(GRID_VERTICES);
}

VOID Sendai::RenderGrid(Sendai::Renderer &Renderer, Sendai::MeshConstants &MeshConstants)
{
    Renderer.CommandList->SetGraphicsRootSignature(Renderer.RootSignGrid.Get());
    Renderer.CommandList->SetPipelineState(Renderer.PipelineState[Sendai::ERenderState::ERS_GRID].Get());
    Renderer.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    MeshConstants.MVP.Model = XMMatrixIdentity();
    std::memcpy(Renderer.MeshDataUploadBufferCpuAddress + Renderer.MeshDataOffset, &MeshConstants,
                sizeof(Sendai::MeshConstants));

    Renderer.CommandList->SetGraphicsRootConstantBufferView(
        0, M_GpuAddress(Renderer.MeshDataUploadBuffer.Get(), Renderer.MeshDataOffset));

    D3D12_VERTEX_BUFFER_VIEW VBV = {
        .BufferLocation = Renderer.GridBufferLocation,
        .SizeInBytes = sizeof(GRID_VERTICES),
        .StrideInBytes = sizeof(XMFLOAT3),
    };

    Renderer.CommandList->IASetVertexBuffers(0, 1, &VBV);
    Renderer.CommandList->DrawInstanced(GRID_VERTICES_COUNT, 1, 0, 0);

    Renderer.MeshDataOffset += CB_ALIGN(MeshConstants);
}