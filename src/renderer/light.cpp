#include "../core/pch.h"

#include "billboard.h"
#include "light.h"

#include "../core/camera.h"
#include "../core/engine.h"
#include "../core/memory.h"
#include "../core/scene.h"
#include "../shaders/sendai/shader_defs.h"

static void RenderLightBillboard(const R_MeshConstants *const MeshConstants, R_Core *const Renderer, XMFLOAT3 Tint);

void
R_LightsInit(S_Scene *const Scene, const Sendai::Camera *const Camera)
{
	Scene->ActiveLightMask = 0;
	Scene->ActiveLightMask |= (1 << 0);
	Scene->Data = {
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
	  .CameraPosition = Camera->GetPosition(),
	};
}

void
R_UpdateLights(BYTE ActiveLightMask, const R_Light *const InLights, R_Light *const OutLights, UINT NumLights)
{
	for (UINT i = 0; i < NumLights; i++) {
		if (IS_LIGHT_ACTIVE(ActiveLightMask, i)) {
			OutLights[i].LightColor = InLights[i].LightColor;
			OutLights[i].LightPosition = InLights[i].LightPosition;
		}
	}
}

void
R_RenderLightBillboards(R_Core *const Renderer, const R_Light *Lights, BYTE ActiveLightMask, R_MeshConstants *const MeshConstants)
{
	Renderer->CommandList->SetGraphicsRootSignature(Renderer->RootSignBillboard.Get());
	Renderer->CommandList->SetPipelineState(Renderer->PipelineState[ERS_BILLBOARD].Get());
	Renderer->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D12_GPU_DESCRIPTOR_HANDLE LampTextureHandle =	Renderer->TexturesHeap->GetGPUDescriptorHandleForHeapStart();
	LampTextureHandle.ptr += (UINT64)ERSI_BILLBOARD_LAMP * Renderer->DescriptorHandleIncrementSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	Renderer->CommandList->SetGraphicsRootDescriptorTable(1, LampTextureHandle);

	for (int i = 0; i < PBR_MAX_LIGHT_NUMBER; ++i) {
		if (ActiveLightMask & (1 << i)) {
			XMVECTOR LightPos = XMLoadFloat3(&Lights[i].LightPosition);
			MeshConstants->MVP.Model = XM_MAT_TRANSLATION_FROM_VEC(LightPos);
			RenderLightBillboard(MeshConstants, Renderer, Lights[i].LightColor);
		}
	}
}

void
RenderLightBillboard(const R_MeshConstants *const MeshConstants, R_Core *const Renderer, XMFLOAT3 Tint)
{
	R_LightBillboardConstants CB = {.MVP = MeshConstants->MVP, .Tint = Tint};
	memcpy(Renderer->MeshDataUploadBufferCpuAddress + Renderer->MeshDataOffset, &CB, sizeof(R_LightBillboardConstants));

	Renderer->CommandList->SetGraphicsRootConstantBufferView(0, M_GpuAddress(Renderer->MeshDataUploadBuffer.Get(), Renderer->MeshDataOffset));

	D3D12_VERTEX_BUFFER_VIEW VBV = {
	  .BufferLocation = Renderer->BillboardBufferLocation,
	  .SizeInBytes = sizeof(BillboardVertices),
	  .StrideInBytes = sizeof(BillboardVertex),
	};
	Renderer->CommandList->IASetVertexBuffers(0, 1, &VBV);
	Renderer->CommandList->DrawInstanced(4, 1, 0, 0);

	Renderer->MeshDataOffset += CB_ALIGN(R_LightBillboardConstants);
}