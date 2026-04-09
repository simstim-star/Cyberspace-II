#pragma once

#include "../core/pch.h"


#include "shader.h"
#include "renderer.h"
#include "render_types.h"
#include "../core/log.h"
#include "../error/error.h"
#include "../win32/win_path.h"
#include "../shaders/sendai/shader_defs.h"

using namespace DirectX;

HRESULT
Sendai::CompileShader(std::wstring &FilePath, ID3DBlob **Blob, EShaderType ShaderType)
{
#if defined(_DEBUG)
    const UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    const UINT CompileFlags = 0;
#endif
    ID3DBlob *ErrorBlob = NULL;
    HRESULT hr = S_OK;

    switch (ShaderType)
    {
    case EST_VERTEX_SHADER:
        hr = D3DCompileFromFile(FilePath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1",
                                CompileFlags, 0, Blob, &ErrorBlob);
        break;
    case EST_PIXEL_SHADER:
        hr = D3DCompileFromFile(FilePath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1",
                                CompileFlags, 0, Blob, &ErrorBlob);
        break;
    }

    return hr;
}

VOID Sendai::CreatePBRPipelineState(Sendai::Renderer *Renderer)
{
    D3D12_ROOT_PARAMETER RootParameters[4] = {};

    // MeshData (b0)
    RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParameters[0].Descriptor.ShaderRegister = 0;
    RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // PBRData (b1)
    RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    RootParameters[1].Constants.ShaderRegister = 1;
    RootParameters[1].Constants.Num32BitValues = Sendai::NUM_32BITS_PBR_VALUES;
    RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // SceneData (b2)
    RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParameters[2].Descriptor.ShaderRegister = 2;
    RootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Texture Table (Bindless)
    D3D12_DESCRIPTOR_RANGE SrvRange = {
        .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        .NumDescriptors = PBR_N_TEXTURES_DESCRIPTORS,
        .BaseShaderRegister = 0,
        .RegisterSpace = 1,
        .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
    };

    RootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
    RootParameters[3].DescriptorTable.pDescriptorRanges = &SrvRange;
    RootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Sampler
    D3D12_STATIC_SAMPLER_DESC Sampler = {
        .Filter = D3D12_FILTER_ANISOTROPIC,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
        .MinLOD = 0.0f,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {
        .NumParameters = _countof(RootParameters),
        .pParameters = RootParameters,
        .NumStaticSamplers = 1,
        .pStaticSamplers = &Sampler,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                 D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED,
    };

    ID3DBlob *Signature = NULL;
    ID3DBlob *Error = NULL;
    HRESULT hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);

    if (FAILED(hr))
    {
        if (Error)
        {
            const char *ErrorMsg = static_cast<const char *>(Error->GetBufferPointer());
            std::string s(ErrorMsg);
            std::wstring ws(s.begin(), s.end());
            Sendai::LOG.Append(static_cast<PWSTR>(Error->GetBufferPointer()));
        }
        ExitIfFailed(hr);
    }

    hr = Renderer->Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(),
                                               IID_PPV_ARGS(Renderer->RootSignPBR.GetAddressOf()));
    ExitIfFailed(hr);

    std::wstring GLTFShadersPath = Win32FullPath(L"/shaders/sendai/pbr.hlsl");
    ID3DBlob *VS = NULL;
    hr = CompileShader(GLTFShadersPath, &VS, EST_VERTEX_SHADER);
    ExitIfFailed(hr);
    ID3DBlob *PS = NULL;
    hr = CompileShader(GLTFShadersPath, &PS, EST_PIXEL_SHADER);
    ExitIfFailed(hr);

    const D3D12_INPUT_ELEMENT_DESC InputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {
        .pRootSignature = Renderer->RootSignPBR.Get(),
        .VS =
            {
                .pShaderBytecode = VS->GetBufferPointer(),
                .BytecodeLength = VS->GetBufferSize(),
            },
        .PS =
            {
                .pShaderBytecode = PS->GetBufferPointer(),
                .BytecodeLength = PS->GetBufferSize(),
            },
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .InputLayout = {.pInputElementDescs = InputElementDescs, .NumElements = _countof(InputElementDescs)},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc{
            .Count = 1,
        }};
    PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    hr = Renderer->Device->CreateGraphicsPipelineState(
        &PSODesc, IID_PPV_ARGS(Renderer->PipelineState[Sendai::ERenderState::ERS_GLTF].GetAddressOf()));
    ExitIfFailed(hr);

    PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    hr = Renderer->Device->CreateGraphicsPipelineState(
        &PSODesc, IID_PPV_ARGS(Renderer->PipelineState[Sendai::ERenderState::ERS_WIREFRAME].GetAddressOf()));
    ExitIfFailed(hr);
}

VOID Sendai::CreateBillboardPipelineState(Sendai::Renderer *Renderer)
{
    auto LightShadersPath = Win32FullPath(L"/shaders/sendai/billboard.hlsl");
    ID3DBlob *VS = NULL;
    HRESULT hr = CompileShader(LightShadersPath, &VS, EST_VERTEX_SHADER);
    ExitIfFailed(hr);
    ID3DBlob *PS = NULL;
    hr = CompileShader(LightShadersPath, &PS, EST_PIXEL_SHADER);
    ExitIfFailed(hr);

    D3D12_ROOT_PARAMETER RootParameters[2];

    // MeshData (b0)
    RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParameters[0].Descriptor.ShaderRegister = 0;
    RootParameters[0].Descriptor.RegisterSpace = 0;
    RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Texture (t0)
    D3D12_DESCRIPTOR_RANGE Range = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    RootParameters[1].DescriptorTable.pDescriptorRanges = &Range;
    RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Sampler
    D3D12_STATIC_SAMPLER_DESC Sampler = {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .ShaderRegister = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {
        .NumParameters = _countof(RootParameters),
        .pParameters = RootParameters,
        .NumStaticSamplers = 1,
        .pStaticSamplers = &Sampler,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ID3DBlob *Signature = NULL;
    ID3DBlob *Error = NULL;
    hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);

    if (FAILED(hr))
    {
        if (Error)
        {
            Sendai::LOG.Append(static_cast<PWSTR>(Error->GetBufferPointer()));
        }
        ExitIfFailed(hr);
    }

    hr = Renderer->Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(),
                                               IID_PPV_ARGS(Renderer->RootSignBillboard.GetAddressOf()));

    const D3D12_INPUT_ELEMENT_DESC InputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {
        .pRootSignature = Renderer->RootSignBillboard.Get(),
        .VS = {.pShaderBytecode = VS->GetBufferPointer(), .BytecodeLength = VS->GetBufferSize()},
        .PS = {.pShaderBytecode = PS->GetBufferPointer(), .BytecodeLength = PS->GetBufferSize()},
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = {.pInputElementDescs = InputElementDescs, .NumElements = _countof(InputElementDescs)},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = {.Count = 1},
    };
    PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM, PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    D3D12_RENDER_TARGET_BLEND_DESC TransparencyBlend = {
        .BlendEnable = TRUE,
        .LogicOpEnable = FALSE,
        .SrcBlend = D3D12_BLEND_SRC_ALPHA,
        .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    PSODesc.BlendState.RenderTarget[0] = TransparencyBlend;

    PSODesc.DepthStencilState = {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = FALSE,
    };

    hr = Renderer->Device->CreateGraphicsPipelineState(
        &PSODesc, IID_PPV_ARGS(Renderer->PipelineState[Sendai::ERenderState::ERS_BILLBOARD].GetAddressOf()));
    ExitIfFailed(hr);
}

VOID Sendai::CreateGridPipelineState(Sendai::Renderer *Renderer)
{
    auto ShadersPath = Win32FullPath(L"/shaders/sendai/grid.hlsl");
    ID3DBlob *VS = NULL;
    HRESULT hr = CompileShader(ShadersPath, &VS, EST_VERTEX_SHADER);
    ExitIfFailed(hr);
    ID3DBlob *PS = NULL;
    hr = CompileShader(ShadersPath, &PS, EST_PIXEL_SHADER);
    ExitIfFailed(hr);

    D3D12_ROOT_PARAMETER RootParameters[1];

    // MeshData (b0)
    RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    RootParameters[0].Descriptor.ShaderRegister = 0;
    RootParameters[0].Descriptor.RegisterSpace = 0;
    RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {
        .NumParameters = _countof(RootParameters),
        .pParameters = RootParameters,
        .NumStaticSamplers = 0,
        .pStaticSamplers = NULL,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ID3DBlob *Signature = NULL;
    ID3DBlob *Error = NULL;
    hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);

    if (FAILED(hr))
    {
        if (Error)
        {
            Sendai::LOG.Append(static_cast<PWSTR>(Error->GetBufferPointer()));
        }
        ExitIfFailed(hr);
    }

    hr = Renderer->Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(),
                                               IID_PPV_ARGS(Renderer->RootSignGrid.GetAddressOf()));

    const D3D12_INPUT_ELEMENT_DESC InputElementDescs[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                                                           D3D12_APPEND_ALIGNED_ELEMENT,
                                                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {
        .pRootSignature = Renderer->RootSignGrid.Get(),
        .VS = {.pShaderBytecode = VS->GetBufferPointer(), .BytecodeLength = VS->GetBufferSize()},
        .PS = {.pShaderBytecode = PS->GetBufferPointer(), .BytecodeLength = PS->GetBufferSize()},
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = {.pInputElementDescs = InputElementDescs, .NumElements = _countof(InputElementDescs)},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
        .NumRenderTargets = 1,
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = {.Count = 1},
    };
    PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM, PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    D3D12_RENDER_TARGET_BLEND_DESC TransparencyBlend = {
        .BlendEnable = TRUE,
        .LogicOpEnable = FALSE,
        .SrcBlend = D3D12_BLEND_SRC_ALPHA,
        .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    PSODesc.BlendState.RenderTarget[0] = TransparencyBlend;

    PSODesc.DepthStencilState = {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = FALSE,
    };

    hr = Renderer->Device->CreateGraphicsPipelineState(
        &PSODesc, IID_PPV_ARGS(Renderer->PipelineState[Sendai::ERenderState::ERS_GRID].GetAddressOf()));
    ExitIfFailed(hr);
}

XMMATRIX
Sendai::NormalMatrix(XMFLOAT4X4 *Model)
{
    XMMATRIX ModelMatrix = XMLoadFloat4x4(Model);
    XMMATRIX ModelInv = XMMatrixInverse(NULL, ModelMatrix);
    return XMMatrixTranspose(ModelInv);
}