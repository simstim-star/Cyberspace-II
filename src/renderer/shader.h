#pragma once

struct Sendai::Renderer;
struct ID3D12Device;
struct ID3D12RootSignature;

typedef enum EShaderType {
	EST_VERTEX_SHADER,
	EST_PIXEL_SHADER,
} EShaderType;

HRESULT R_CompileShader(std::wstring &FilePath, ID3DBlob **Blob, EShaderType ShaderType);
void R_CreatePBRPipelineState(Sendai::Renderer *Renderer);
void R_CreateBillboardPipelineState(Sendai::Renderer *Renderer);
void R_CreateGridPipelineState(Sendai::Renderer *Renderer);
XMMATRIX R_NormalMatrix(XMFLOAT4X4 *Model);
