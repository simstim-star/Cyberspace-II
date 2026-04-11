#include "../core/pch.h"

#include "../core/log.h"
#include "../renderer/renderer.h"
#include "../win32/win_path.h"
#include "asset_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3dx12_barriers.h>

using namespace Sendai;
namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::wstring;

constexpr char EMBEDDED_TEXTURE_SYMBOL = '*';

UINT _ResolveAndLoadTexture(const wstring &ModelFolder, aiMaterial *Material, aiTextureType Type, const aiScene *Scene,
                            Renderer *Renderer);
VOID _ProcessNode(aiNode *Node, const aiScene *Scene, Model &Model, Renderer *Renderer, wstring &ModelFolder);
VOID _ProcessMesh(aiMesh *Mesh, const aiScene *Scene, Model &Model, Renderer *Renderer, wstring &ModelFolder);
VOID _UploadToGpu(Renderer *Renderer, Primitive &Primitive, const vector<Vertex> &Vertices,
                  const vector<uint32_t> &Indices);

BOOL Sendai::LoadModel(Renderer *Renderer, const wstring &PathW, Scene &Scene)
{
    Assimp::Importer Importer;
    string Path(PathW.begin(), PathW.end());
    UINT Flags = aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_CalcTangentSpace |
                 aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals |
                 aiProcess_PreTransformVertices;

    const aiScene *pScene = Importer.ReadFile(Path, Flags);

    if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
    {
        string Error = Importer.GetErrorString();
        wstring wErr(Error.begin(), Error.end());
        Sendai::LOG.Appendf(L"Assimp Error: %S\n", wErr.c_str());
        return FALSE;
    }

    Sendai::Model &Model = Scene.Models.back();
    Model.Scale = {1.0f, 1.0f, 1.0f};
    Model.bVisible = TRUE;
    Model.Name = Win32GetFileNameOnly(PathW);

    fs::path FullPath(PathW);
    wstring ModelFolder = FullPath.parent_path().wstring();
    _ProcessNode(pScene->mRootNode, pScene, Model, Renderer, ModelFolder);

    CD3DX12_RESOURCE_BARRIER Barriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(Renderer->VertexBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                             D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition(Renderer->IndexBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                             D3D12_RESOURCE_STATE_INDEX_BUFFER)};
    Renderer->CommandList->ResourceBarrier(2, Barriers);

    Sendai::LOG.Appendf(L"Successfully loaded model: %s\n", PathW.c_str());
    return TRUE;
}

VOID _ProcessNode(aiNode *Node, const aiScene *Scene, Sendai::Model &Model, Sendai::Renderer *Renderer,
                  std::wstring &ModelFolder)
{
    for (auto i = 0; i < Node->mNumMeshes; i++)
    {
        aiMesh *Mesh = Scene->mMeshes[Node->mMeshes[i]];
        _ProcessMesh(Mesh, Scene, Model, Renderer, ModelFolder);
    }

    for (auto i = 0; i < Node->mNumChildren; i++)
    {
        _ProcessNode(Node->mChildren[i], Scene, Model, Renderer, ModelFolder);
    }
}

UINT _ResolveAndLoadTexture(const wstring &ModelFolder, aiMaterial *Material, aiTextureType Type, const aiScene *Scene,
                            Renderer *Renderer)
{
    aiString TexturePath;
    if (Material->GetTexture(Type, 0, &TexturePath) != AI_SUCCESS)
    {
        return 0;
    }

    const CHAR *RawTexturePath = TexturePath.C_Str();

    if (RawTexturePath[0] == EMBEDDED_TEXTURE_SYMBOL)
    {
        INT Index = std::atoi(&RawTexturePath[1]);
        if (Index < (INT)Scene->mNumTextures)
        {
            aiTexture *pEmbedded = Scene->mTextures[Index];
            return Renderer->Textures->LoadEmbedded(pEmbedded);
        }
    }

    string Path(RawTexturePath);
    wstring wPath(Path.begin(), Path.end());
    wstring FullPath = ModelFolder + L"/" + wPath;

    return Renderer->Textures->Load(Renderer->Device.Get(), FullPath);
}

VOID _ProcessMesh(aiMesh *Mesh, const aiScene *Scene, Model &Model, Renderer *Renderer, wstring &ModelFolder)
{
    /* Vertices */
    vector<Sendai::Vertex> Vertices;
    Vertices.reserve(Mesh->mNumVertices);
    for (UINT i = 0; i < Mesh->mNumVertices; i++)
    {
        Sendai::Vertex Vertex{};
        Vertex.Position = {Mesh->mVertices[i].x, Mesh->mVertices[i].y, Mesh->mVertices[i].z};
        UINT uvComponents = Mesh->mNumUVComponents[0];

        if (Mesh->HasNormals())
        {
            Vertex.Normal = {Mesh->mNormals[i].x, Mesh->mNormals[i].y, Mesh->mNormals[i].z};
        }
        if (Mesh->HasTangentsAndBitangents())
        {
            Vertex.Tangent = {Mesh->mTangents[i].x, Mesh->mTangents[i].y, Mesh->mTangents[i].z, 1.0f};
        }

        if (Mesh->mTextureCoords[0])
        {
            Vertex.UV0 = {Mesh->mTextureCoords[0][i].x, Mesh->mTextureCoords[0][i].y};
        }

        if (Mesh->mTextureCoords[1])
        {
            Vertex.UV1 = {Mesh->mTextureCoords[1][i].x, Mesh->mTextureCoords[1][i].y};
        }

        Vertices.push_back(Vertex);
    }

    /* Indices */
    std::vector<uint32_t> Indices;
    Indices.reserve(Mesh->mNumFaces);
    for (UINT i = 0; i < Mesh->mNumFaces; i++)
    {
        aiFace Face = Mesh->mFaces[i];
        for (UINT j = 0; j < Face.mNumIndices; j++)
        {
            Indices.push_back(Face.mIndices[j]);
        }
    }

    Sendai::Primitive Primitive{};
    Primitive.IndexCount = (UINT)Indices.size();

    /* Materials */
    if (Mesh->mMaterialIndex >= 0)
    {
        aiMaterial *Material = Scene->mMaterials[Mesh->mMaterialIndex];

        Primitive.ConstantBuffer.UVOffset = {0.0f, 0.0f};
        Primitive.ConstantBuffer.UVScale = {1.0f, 1.0f};
        Primitive.ConstantBuffer.UVRotation = 0.0f;

        aiUVTransform Transform;
        if (Material->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_BASE_COLOR, 1), Transform) == AI_SUCCESS)
        {
            Primitive.ConstantBuffer.UVOffset = {Transform.mTranslation.x, Transform.mTranslation.y};
            Primitive.ConstantBuffer.UVScale = {Transform.mScaling.x, Transform.mScaling.y};
            Primitive.ConstantBuffer.UVRotation = Transform.mRotation;
        }

        aiColor4D BaseColor(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(Material, AI_MATKEY_BASE_COLOR, &BaseColor);
        Primitive.ConstantBuffer.BaseColorFactor = {BaseColor.r, BaseColor.g, BaseColor.b, BaseColor.a};

        FLOAT RoughnessFactor = 1.0f;
        aiGetMaterialFloat(Material, AI_MATKEY_ROUGHNESS_FACTOR, &RoughnessFactor);
        FLOAT MetallicFactor = 1.0f;
        aiGetMaterialFloat(Material, AI_MATKEY_METALLIC_FACTOR, &MetallicFactor);

        Primitive.ConstantBuffer.RoughnessFactor = RoughnessFactor;
        Primitive.ConstantBuffer.MetallicFactor = MetallicFactor;

        Primitive.ConstantBuffer.AlbedoTextureIndex =
            _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_DIFFUSE, Scene, Renderer);
        if (Primitive.ConstantBuffer.AlbedoTextureIndex == 0)
        {
            Primitive.ConstantBuffer.AlbedoTextureIndex =
                _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_BASE_COLOR, Scene, Renderer);
        }

        Primitive.ConstantBuffer.NormalTextureIndex =
            _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_NORMALS, Scene, Renderer);
        if (Primitive.ConstantBuffer.NormalTextureIndex == 0)
        {
            Primitive.ConstantBuffer.NormalTextureIndex =
                _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_HEIGHT, Scene, Renderer);
        }

        Primitive.ConstantBuffer.MetallicTextureIndex =
            _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_METALNESS, Scene, Renderer);

        Primitive.ConstantBuffer.EmissiveTextureIndex =
            _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_EMISSIVE, Scene, Renderer);

        Primitive.ConstantBuffer.OcclusionTextureIndex =
            _ResolveAndLoadTexture(ModelFolder, Material, aiTextureType_AMBIENT_OCCLUSION, Scene, Renderer);
    }

    _UploadToGpu(Renderer, Primitive, Vertices, Indices);

    Model.Primitives.push_back(Primitive);
}

VOID _UploadToGpu(Renderer *Renderer, Primitive &Primitive, const vector<Vertex> &Vertices,
                  const vector<uint32_t> &Indices)
{

    /* Vertex */

    const auto VertexBufferSize = Vertices.size() * sizeof(Sendai::Vertex);
    const auto VertexOffset = Renderer->GeometryUploadBuffer.CurrentOffset();
    Primitive.VertexBufferView.BufferLocation = Renderer->GeometryUploadBuffer.PushBack(Vertices);
    Primitive.VertexBufferView.StrideInBytes = sizeof(Sendai::Vertex);
    Primitive.VertexBufferView.SizeInBytes = VertexBufferSize;
    Renderer->CommandList->CopyBufferRegion(Renderer->VertexBufferDefault.Get(), Renderer->CurrentVertexBufferOffset,
                                            Renderer->GeometryUploadBuffer.Get(), VertexOffset, VertexBufferSize);
    Renderer->CurrentVertexBufferOffset += VertexBufferSize;

    /* Index */

    const auto IndexBufferSize = Indices.size() * sizeof(uint32_t);
    const auto IndexOffset = Renderer->GeometryUploadBuffer.CurrentOffset();
    Primitive.IndexBufferView.BufferLocation = Renderer->GeometryUploadBuffer.PushBack(Indices);
    Primitive.IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    Primitive.IndexBufferView.SizeInBytes = IndexBufferSize;

    /* Record copy Upload->Default */
    Renderer->CommandList->CopyBufferRegion(Renderer->IndexBufferDefault.Get(), Renderer->CurrentIndexBufferOffset,
                                            Renderer->GeometryUploadBuffer.Get(), IndexOffset, IndexBufferSize);

    Renderer->CurrentIndexBufferOffset += IndexBufferSize;
}