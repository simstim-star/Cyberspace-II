#pragma once

#include "DirectXMathC.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace Sendai {

struct Vertex {
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Tangent;
	XMFLOAT2 UV0;
	XMFLOAT2 UV1;
};

struct PBRConstantBuffer {
	XMFLOAT4 BaseColorFactor;
	FLOAT MetallicFactor;
	FLOAT RoughnessFactor;
	FLOAT Padding0[2];

	XMFLOAT3 EmissiveFactor;
	FLOAT Padding1;
	FLOAT XXX;

	XMFLOAT2 UVOffset;
	XMFLOAT2 UVScale;
	FLOAT UVRotation;

	UINT32 AlbedoTextureIndex;
	UINT32 NormalTextureIndex;
	UINT32 MetallicTextureIndex;
	UINT32 OcclusionTextureIndex;
	UINT32 EmissiveTextureIndex;
};

constexpr size_t NUM_32BITS_PBR_VALUES = sizeof(PBRConstantBuffer) / 4;

struct Light {
	XMFLOAT3 LightPosition;
	FLOAT Padding0;
	XMFLOAT3 LightColor;
	FLOAT Padding1;
};

struct SceneData {
	Light Lights[7];

	XMFLOAT3 CameraPosition;
	FLOAT Padding0;
};

struct MVP {
	XMMATRIX Model;
	XMMATRIX View;
	XMMATRIX Proj;
};

struct MeshConstants {
	MVP MVP;
	XMMATRIX Normal;
};

struct LightBillboardConstants {
	MVP MVP;
	XMFLOAT3 Tint;
};

struct Texture {
	INT Width;
	INT Height;
	INT Channels;
	const wchar_t *Name;
	size_t Size;
	std::array<const UINT8 *, D3D12_REQ_MIP_LEVELS> MipPixels;
	UINT MipLevels;
};

struct Primitive {
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	UINT IndexCount;

	PBRConstantBuffer ConstantBuffer;
};

struct Mesh {
	std::vector<Primitive> Primitives;
};

struct Node {
	std::unique_ptr<Mesh> MeshData;

	// RH and Row-Major
	XMFLOAT4X4 ModelMatrix;
};

struct Model {
	std::wstring Name;

	std::vector<Node> Nodes;
	std::vector<Texture> Images;

	XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
	XMFLOAT3 Rotation{0.0f, 0.0f, 0.0f};
	XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};

	BOOL Visible;
};

} // namespace Sendai
