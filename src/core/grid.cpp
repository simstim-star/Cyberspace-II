#include "pch.h"

#include "../renderer/render_types.h"
#include "../renderer/renderer.h"
#include "grid.h"

#include <DirectXMathC.h>

#define GRID_VERTICES_COUNT 300
static XMFLOAT3 GRID_VERTICES[GRID_VERTICES_COUNT] = {0};

void
R_CreateGrid(Sendai::Renderer *const Renderer, const float HalfSide)
{
	const INT LinesPerDirection = (GRID_VERTICES_COUNT / 4);
	const FLOAT Step = (HalfSide * 2.0f) / (float)(LinesPerDirection - 1);

	for (INT Line = 0; Line < LinesPerDirection; Line++) {
		FLOAT Position = -HalfSide + (Line * Step);
		INT i = Line * 4;
		GRID_VERTICES[i] = {Position, 0.0f, -HalfSide};
		GRID_VERTICES[i + 1] = {Position, 0.0f, HalfSide};
		GRID_VERTICES[i + 2] = {-HalfSide, 0.0f, Position};
		GRID_VERTICES[i + 3] = {HalfSide, 0.0f, Position};
	}

	std::memcpy(Renderer->SceneDataUploadBufferCpuAddress + Renderer->SceneDataOffset, &GRID_VERTICES, sizeof(GRID_VERTICES));
	Renderer->GridBufferLocation = M_GpuAddress(Renderer->SceneDataUploadBuffer.Get(), Renderer->SceneDataOffset);
	Renderer->SceneDataOffset += CB_ALIGN(GRID_VERTICES);
}

void
R_RenderGrid(Sendai::Renderer *const Renderer, Sendai::MeshConstants *const MeshConstants)
{
	Renderer->CommandList->SetGraphicsRootSignature(Renderer->RootSignGrid.Get());
	Renderer->CommandList->SetPipelineState(Renderer->PipelineState[Sendai::ERenderState::ERS_GRID].Get());
	Renderer->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	MeshConstants->MVP.Model = XMMatrixIdentity();
	std::memcpy(Renderer->MeshDataUploadBufferCpuAddress + Renderer->MeshDataOffset, MeshConstants, sizeof(Sendai::MeshConstants));

	Renderer->CommandList->SetGraphicsRootConstantBufferView(0, M_GpuAddress(Renderer->MeshDataUploadBuffer.Get(), Renderer->MeshDataOffset));

	D3D12_VERTEX_BUFFER_VIEW VBV = {
	  .BufferLocation = Renderer->GridBufferLocation,
	  .SizeInBytes = sizeof(GRID_VERTICES),
	  .StrideInBytes = sizeof(XMFLOAT3),
	};

	Renderer->CommandList->IASetVertexBuffers(0, 1, &VBV);
	Renderer->CommandList->DrawInstanced(GRID_VERTICES_COUNT, 1, 0, 0);

	Renderer->MeshDataOffset += CB_ALIGN(MeshConstants);
}