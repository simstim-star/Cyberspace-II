#pragma once

#include "DirectXMath.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "texture.h"

namespace Sendai
{

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT4 Tangent;
    DirectX::XMFLOAT2 UV0;
    DirectX::XMFLOAT2 UV1;
};

struct PBRConstantBuffer
{
    DirectX::XMFLOAT4 BaseColorFactor;
    FLOAT MetallicFactor;
    FLOAT RoughnessFactor;
    FLOAT Padding0[2];

    DirectX::XMFLOAT3 EmissiveFactor;
    FLOAT Padding1;

    DirectX::XMFLOAT2 UVOffset;
    DirectX::XMFLOAT2 UVScale;
    FLOAT UVRotation;

    UINT32 AlbedoTextureIndex;
    UINT32 NormalTextureIndex;
    UINT32 MetallicTextureIndex;
    UINT32 OcclusionTextureIndex;
    UINT32 EmissiveTextureIndex;
};

constexpr size_t NUM_32BITS_PBR_VALUES = sizeof(PBRConstantBuffer) / 4;

struct Light
{
    DirectX::XMFLOAT3 LightPosition;
    FLOAT Padding0;
    DirectX::XMFLOAT3 LightColor;
    FLOAT Padding1;
};

struct SceneData
{
    Light Lights[7];

    DirectX::XMFLOAT3 CameraPosition;
    FLOAT Padding0;
};

struct MVP
{
    DirectX::XMMATRIX Model;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Proj;
};

struct MeshConstants
{
    MVP MVP;
    DirectX::XMMATRIX Normal;
};

struct LightBillboardConstants
{
    MVP MVP;
    DirectX::XMFLOAT3 Tint;
};

struct Primitive
{
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    UINT IndexCount;

    PBRConstantBuffer ConstantBuffer;
};

struct Mesh
{
    std::vector<Primitive> Primitives;
};

struct Node
{
    std::unique_ptr<Mesh> MeshData;

    // RH and Row-Major
    DirectX::XMFLOAT4X4 ModelMatrix;
};

struct Model
{
    std::wstring Name;

    std::vector<Primitive> Primitives;

    DirectX::XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 Rotation{0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};

    BOOL bVisible;
};

} // namespace Sendai
