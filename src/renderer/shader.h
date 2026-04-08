#pragma once

struct Sendai::Renderer;
struct ID3D12Device;
struct ID3D12RootSignature;

namespace Sendai {
enum EShaderType {
	EST_VERTEX_SHADER,
	EST_PIXEL_SHADER,
};

HRESULT CompileShader(std::wstring &FilePath, ID3DBlob **Blob, EShaderType ShaderType);
VOID CreatePBRPipelineState(Sendai::Renderer *Renderer);
VOID CreateBillboardPipelineState(Sendai::Renderer *Renderer);
VOID CreateGridPipelineState(Sendai::Renderer *Renderer);
XMMATRIX NormalMatrix(XMFLOAT4X4 *Model);
} // namespace Senai
