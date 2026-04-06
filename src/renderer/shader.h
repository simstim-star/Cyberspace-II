#pragma once

struct R_Core;
struct ID3D12Device;
struct ID3D12RootSignature;

typedef enum EShaderType {
	EST_VERTEX_SHADER,
	EST_PIXEL_SHADER,
} EShaderType;

HRESULT R_CompileShader(std::wstring &FilePath, ID3DBlob **Blob, EShaderType ShaderType);
void R_CreatePBRPipelineState(R_Core *Renderer);
void R_CreateBillboardPipelineState(R_Core *Renderer);
void R_CreateGridPipelineState(R_Core *Renderer);
XMMATRIX R_NormalMatrix(XMFLOAT4X4 *Model);
