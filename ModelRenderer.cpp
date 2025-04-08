#include "ModelRenderer.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Engine.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <cassert>

// カメラの実装
Camera::Camera()
    : m_position(0.0f, 0.0f, -5.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_fov(DirectX::XM_PIDIV4)         // 45度
    , m_aspectRatio(16.0f / 9.0f)       // 16:9
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
{
}

void Camera::SetPosition(const DirectX::XMFLOAT3& position)
{
    m_position = position;
}

void Camera::SetTarget(const DirectX::XMFLOAT3& target)
{
    m_target = target;
}

void Camera::SetUp(const DirectX::XMFLOAT3& up)
{
    m_up = up;
}

void Camera::SetFov(float fov)
{
    m_fov = fov;
}

void Camera::SetAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
}

void Camera::SetClipPlanes(float nearPlane, float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
    DirectX::XMVECTOR eyePosition = DirectX::XMLoadFloat3(&m_position);
    DirectX::XMVECTOR focusPosition = DirectX::XMLoadFloat3(&m_target);
    DirectX::XMVECTOR upDirection = DirectX::XMLoadFloat3(&m_up);

    return DirectX::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const
{
    return DirectX::XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
}

// モデルレンダラーの実装
ModelRenderer::ModelRenderer()
    : m_renderer(nullptr)
    , m_resourceManager(nullptr)
    , m_camera(nullptr)
    , m_constantBufferCPU(nullptr)
    , m_materialConstantBufferCPU(nullptr)
    , m_constantBufferIndex(0)
    , m_materialConstantBufferIndex(0)
    , m_initialized(false)
    , m_batchActive(false)
{
}

ModelRenderer::~ModelRenderer()
{
    Shutdown();
}

bool ModelRenderer::Initialize(Renderer* renderer, ResourceManager* resourceManager)
{
    assert(renderer != nullptr);
    assert(resourceManager != nullptr);

    m_renderer = renderer;
    m_resourceManager = resourceManager;

    // デフォルトカメラの作成
    m_camera = std::make_shared<Camera>();

    // パイプラインステートの作成
    if (!CreatePipelineState()) {
        return false;
    }

    // 定数バッファの作成
    if (!CreateConstantBuffer()) {
        return false;
    }

    m_initialized = true;
    return true;
}

void ModelRenderer::Shutdown()
{
    if (m_constantBufferCPU) {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferCPU = nullptr;
    }

    if (m_materialConstantBufferCPU) {
        m_materialConstantBuffer->Unmap(0, nullptr);
        m_materialConstantBufferCPU = nullptr;
    }

    m_initialized = false;
}

bool ModelRenderer::CreatePipelineState()
{
    // ルートシグネチャの作成
    if (!CreateRootSignature()) {
        return false;
    }

    // コンパイル済みシェーダーの読み込み
    std::shared_ptr<Shader> modelShader = m_resourceManager->LoadShader(
        L"ModelShader",
        L"Shaders/ModelVS.hlsl",
        L"Shaders/ModelPS.hlsl"
    );

    if (!modelShader || !modelShader->vertexShaderBlob || !modelShader->pixelShaderBlob) {
        // シェーダーのコンパイルに失敗した場合は、埋め込みのシェーダーを使用
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        // モデル用頂点シェーダー
        const char* vertexShaderSource = R"(
            cbuffer ModelConstantBuffer : register(b0)
            {
                float4x4 World;
                float4x4 ViewProjection;
                float3 CameraPosition;
                float Padding;
            };
            
            struct VSInput
            {
                float3 Position : POSITION;
                float3 Normal : NORMAL;
                float2 TexCoord : TEXCOORD;
                float3 Tangent : TANGENT;
                float3 Bitangent : BITANGENT;
            };
            
            struct PSInput
            {
                float4 Position : SV_POSITION;
                float3 WorldPosition : POSITION;
                float3 Normal : NORMAL;
                float2 TexCoord : TEXCOORD;
                float3 Tangent : TANGENT;
                float3 Bitangent : BITANGENT;
            };
            
            PSInput main(VSInput input)
            {
                PSInput output;
                
                // ローカル座標からワールド座標への変換
                float4 worldPosition = mul(float4(input.Position, 1.0f), World);
                
                // ワールド座標からクリップ空間への変換
                output.Position = mul(worldPosition, ViewProjection);
                
                // ワールド座標を出力
                output.WorldPosition = worldPosition.xyz;
                
                // 法線をワールド空間に変換（スケーリングを考慮しない場合は転置行列の逆行列を使用）
                output.Normal = normalize(mul(input.Normal, (float3x3)World));
                
                // 接線と従法線をワールド空間に変換
                output.Tangent = normalize(mul(input.Tangent, (float3x3)World));
                output.Bitangent = normalize(mul(input.Bitangent, (float3x3)World));
                
                // テクスチャ座標をそのまま出力
                output.TexCoord = input.TexCoord;
                
                return output;
            }
        )";

        // モデル用ピクセルシェーダー
        const char* pixelShaderSource = R"(
            Texture2D DiffuseMap : register(t0);
            Texture2D NormalMap : register(t1);
            Texture2D SpecularMap : register(t2);
            SamplerState SurfaceSampler : register(s0);
            
            cbuffer MaterialConstantBuffer : register(b1)
            {
                float4 Diffuse;
                float4 Ambient;
                float4 Specular;
                float Shininess;
                float3 Padding;
            };
            
            cbuffer ModelConstantBuffer : register(b0)
            {
                float4x4 World;
                float4x4 ViewProjection;
                float3 CameraPosition;
                float Padding2;
            };
            
            struct PSInput
            {
                float4 Position : SV_POSITION;
                float3 WorldPosition : POSITION;
                float3 Normal : NORMAL;
                float2 TexCoord : TEXCOORD;
                float3 Tangent : TANGENT;
                float3 Bitangent : BITANGENT;
            };
            
            float4 main(PSInput input) : SV_TARGET
            {
                // ライトの方向（定数として定義、実際にはライトバッファから取得すべき）
                float3 lightDirection = normalize(float3(0.5f, -0.5f, 0.5f));
                float3 lightColor = float3(1.0f, 1.0f, 1.0f);
                
                // テクスチャからディフューズカラーを取得
                float4 textureColor = DiffuseMap.Sample(SurfaceSampler, input.TexCoord);
                
                // 法線マップから法線を取得（TBN行列を使用）
                float3 normalFromMap = NormalMap.Sample(SurfaceSampler, input.TexCoord).rgb * 2.0f - 1.0f;
                float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Normal);
                float3 normal = normalize(mul(normalFromMap, TBN));
                
                // 環境光の計算
                float3 ambient = Ambient.rgb * textureColor.rgb;
                
                // 拡散反射の計算
                float NdotL = max(0.0f, dot(normal, -lightDirection));
                float3 diffuse = Diffuse.rgb * textureColor.rgb * NdotL * lightColor;
                
                // スペキュラー反射の計算
                float3 viewDirection = normalize(CameraPosition - input.WorldPosition);
                float3 reflectionDirection = reflect(lightDirection, normal);
                float spec = pow(max(0.0f, dot(viewDirection, reflectionDirection)), Shininess);
                float3 specularFromMap = SpecularMap.Sample(SurfaceSampler, input.TexCoord).rgb;
                float3 specular = Specular.rgb * spec * specularFromMap * lightColor;
                
                // 最終的な色の計算
                float3 finalColor = ambient + diffuse + specular;
                
                return float4(finalColor, textureColor.a * Diffuse.a);
            }
        )";

        // 頂点シェーダーのコンパイル
        HRESULT hr = D3DCompile(
            vertexShaderSource,
            strlen(vertexShaderSource),
            "ModelVS",
            nullptr,
            nullptr,
            "main",
            "vs_5_1",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0,
            &vertexShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return false;
        }

        // ピクセルシェーダーのコンパイル
        hr = D3DCompile(
            pixelShaderSource,
            strlen(pixelShaderSource),
            "ModelPS",
            nullptr,
            nullptr,
            "main",
            "ps_5_1",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0,
            &pixelShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return false;
        }

        // シェーダーをセット
        modelShader = std::make_shared<Shader>();
        modelShader->name = L"EmbeddedModelShader";
        modelShader->vertexShaderBlob = vertexShaderBlob;
        modelShader->pixelShaderBlob = pixelShaderBlob;
    }

    // 入力レイアウトの設定
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // ブレンドステートの設定（アルファブレンド有効）
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // ラスタライザーステートの設定
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // 深度ステンシルステートの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;

    // グラフィックスパイプラインの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(modelShader->vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(modelShader->pixelShaderBlob.Get());
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_renderer->GetBackBufferFormat();
    psoDesc.DSVFormat = m_renderer->GetDepthStencilFormat();
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    // パイプラインステートの作成
    HRESULT hr = m_renderer->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool ModelRenderer::CreateRootSignature()
{
    // ルートパラメータの設定
    CD3DX12_ROOT_PARAMETER rootParameters[5];

    // モデル定数バッファ（b0）
    CD3DX12_DESCRIPTOR_RANGE cbvRange1;
    cbvRange1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    rootParameters[0].InitAsDescriptorTable(1, &cbvRange1);

    // マテリアル定数バッファ（b1）
    CD3DX12_DESCRIPTOR_RANGE cbvRange2;
    cbvRange2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
    rootParameters[1].InitAsDescriptorTable(1, &cbvRange2);

    // ディフューズマップ（t0）
    CD3DX12_DESCRIPTOR_RANGE srvRange1;
    srvRange1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[2].InitAsDescriptorTable(1, &srvRange1);

    // ノーマルマップ（t1）
    CD3DX12_DESCRIPTOR_RANGE srvRange2;
    srvRange2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    rootParameters[3].InitAsDescriptorTable(1, &srvRange2);

    // スペキュラーマップ（t2）
    CD3DX12_DESCRIPTOR_RANGE srvRange3;
    srvRange3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    rootParameters[4].InitAsDescriptorTable(1, &srvRange3);

    // スタティックサンプラーの設定
    CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1];
    staticSamplers[0].Init(
        0,                                  // シェーダーレジスタ
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // フィルタ
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // アドレスU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // アドレスV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP    // アドレスW
    );

    // ルートシグネチャの設定
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(
        _countof(rootParameters),
        rootParameters,
        _countof(staticSamplers),
        staticSamplers,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    // シリアライズされたルートシグネチャの作成
    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error
    );

    if (FAILED(hr)) {
        if (error) {
            OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
        }
        return false;
    }

    // ルートシグネチャの作成
    hr = m_renderer->GetDevice()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool ModelRenderer::CreateConstantBuffer()
{
    // モデル定数バッファのサイズを計算（256バイトアラインメント）
    const UINT constantBufferSize = (sizeof(ModelConstantBuffer) + 255) & ~255;

    // モデル定数バッファの作成
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    HRESULT hr = m_renderer->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)
    );

    if (FAILED(hr)) {
        return false;
    }

    // モデル定数バッファをマップ
    CD3DX12_RANGE readRange(0, 0);
    hr = m_constantBuffer->Map(
        0,
        &readRange,
        reinterpret_cast<void**>(&m_constantBufferCPU)
    );

    if (FAILED(hr)) {
        return false;
    }

    // マテリアル定数バッファのサイズを計算（256バイトアラインメント）
    const UINT materialConstantBufferSize = (sizeof(MaterialConstantBuffer) + 255) & ~255;

    // マテリアル定数バッファの作成
    CD3DX12_RESOURCE_DESC materialBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(materialConstantBufferSize);

    hr = m_renderer->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &materialBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_materialConstantBuffer)
    );

    if (FAILED(hr)) {
        return false;
    }

    // マテリアル定数バッファをマップ
    hr = m_materialConstantBuffer->Map(
        0,
        &readRange,
        reinterpret_cast<void**>(&m_materialConstantBufferCPU)
    );

    if (FAILED(hr)) {
        return false;
    }

    // CBV/SRV/UAVヒープにCBVを作成
    m_constantBufferIndex = 1; // エンジンから適切なインデックスを取得する必要がある
    m_materialConstantBufferIndex = 2;

    // モデル定数バッファビューの設定
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constantBufferSize;

    ID3D12DescriptorHeap* heap = m_resourceManager->GetCbvSrvUavHeap();
    if (heap) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_constantBufferIndex,
            m_renderer->GetCbvSrvUavDescriptorSize()
        );

        m_renderer->GetDevice()->CreateConstantBufferView(&cbvDesc, handle);
    }

    // マテリアル定数バッファビューの設定
    D3D12_CONSTANT_BUFFER_VIEW_DESC materialCbvDesc = {};
    materialCbvDesc.BufferLocation = m_materialConstantBuffer->GetGPUVirtualAddress();
    materialCbvDesc.SizeInBytes = materialConstantBufferSize;

    if (heap) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
            heap->GetCPUDescriptorHandleForHeapStart(),
            m_materialConstantBufferIndex,
            m_renderer->GetCbvSrvUavDescriptorSize()
        );

        m_renderer->GetDevice()->CreateConstantBufferView(&materialCbvDesc, handle);
    }

    return true;
}

void ModelRenderer::SetCamera(const std::shared_ptr<Camera>& camera)
{
    if (camera) {
        m_camera = camera;
    }
}

void ModelRenderer::Begin()
{
    if (!m_initialized || m_batchActive) {
        return;
    }

    // コマンドリストの設定
    ID3D12GraphicsCommandList* commandList = m_renderer->GetCommandList();

    // パイプラインステートとルートシグネチャの設定
    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    // プリミティブトポロジーの設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ディスクリプタヒープの設定
    ID3D12DescriptorHeap* heaps[] = { m_resourceManager->GetCbvSrvUavHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // レンダーターゲットの設定
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_renderer->GetRtvHeap()->GetCPUDescriptorHandleForHeapStart(),
        m_renderer->GetCommandList()->GetCurrentBackBufferIndex(),
        m_renderer->GetRtvDescriptorSize()
    );

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(
        m_renderer->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart()
    );

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // バッチアクティブフラグを設定
    m_batchActive = true;
}

void ModelRenderer::End()
{
    if (!m_initialized || !m_batchActive) {
        return;
    }

    // バッチアクティブフラグをリセット
    m_batchActive = false;
}

void ModelRenderer::DrawModel(const std::shared_ptr<Model>& model, const DirectX::XMMATRIX& worldMatrix)
{
    if (!m_initialized || !model) {
        return;
    }

    // モデルの各メッシュを描画
    for (const auto& mesh : model->meshes) {
        DrawMesh(mesh, worldMatrix);
    }
}

void ModelRenderer::DrawMesh(const std::shared_ptr<Mesh>& mesh, const DirectX::XMMATRIX& worldMatrix)
{
    if (!m_initialized || !mesh) {
        return;
    }

    // ワールド・ビュー・プロジェクション行列の更新
    UpdateWorldViewProjection(worldMatrix);

    // マテリアルの更新
    UpdateMaterial(mesh->material);

    ID3D12GraphicsCommandList* commandList = m_renderer->GetCommandList();

    // モデル定数バッファビューの設定
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(
        m_resourceManager->GetCbvSrvUavHeap()->GetGPUDescriptorHandleForHeapStart(),
        m_constantBufferIndex,
        m_renderer->GetCbvSrvUavDescriptorSize()
    );

    commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

    // マテリアル定数バッファビューの設定
    CD3DX12_GPU_DESCRIPTOR_HANDLE materialCbvHandle(
        m_resourceManager->GetCbvSrvUavHeap()->GetGPUDescriptorHandleForHeapStart(),
        m_materialConstantBufferIndex,
        m_renderer->GetCbvSrvUavDescriptorSize()
    );

    commandList->SetGraphicsRootDescriptorTable(1, materialCbvHandle);

    // テクスチャの設定
    if (mesh->material) {
        // ディフューズマップの設定
        if (mesh->material->diffuseMap) {
            CD3DX12_GPU_DESCRIPTOR_HANDLE diffuseHandle(
                m_resourceManager->GetCbvSrvUavHeap()->GetGPUDescriptorHandleForHeapStart(),
                mesh->material->diffuseMap->descriptorIndex,
                m_renderer->GetCbvSrvUavDescriptorSize()
            );

            commandList->SetGraphicsRootDescriptorTable(2, diffuseHandle);
        }

        // ノーマルマップの設定
        if (mesh->material->normalMap) {
            CD3DX12_GPU_DESCRIPTOR_HANDLE normalHandle(
                m_resourceManager->GetCbvSrvUavHeap()->GetGPUDescriptorHandleForHeapStart(),
                mesh->material->normalMap->descriptorIndex,
                m_renderer->GetCbvSrvUavDescriptorSize()
            );

            commandList->SetGraphicsRootDescriptorTable(3, normalHandle);
        }

        // スペキュラーマップの設定
        if (mesh->material->specularMap) {
            CD3DX12_GPU_DESCRIPTOR_HANDLE specularHandle(
                m_resourceManager->GetCbvSrvUavHeap()->GetGPUDescriptorHandleForHeapStart(),
                mesh->material->specularMap->descriptorIndex,
                m_renderer->GetCbvSrvUavDescriptorSize()
            );

            commandList->SetGraphicsRootDescriptorTable(4, specularHandle);
        }
    }

    // 頂点バッファとインデックスバッファの設定
    commandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
    commandList->IASetIndexBuffer(&mesh->indexBufferView);

    // 描画コマンドの発行
    commandList->DrawIndexedInstanced(mesh->indexCount, 1, 0, 0, 0);
}

void ModelRenderer::UpdateWorldViewProjection(const DirectX::XMMATRIX& worldMatrix)
{
    if (!m_constantBufferCPU || !m_camera) {
        return;
    }

    // ビュー行列の取得
    DirectX::XMMATRIX viewMatrix = m_camera->GetViewMatrix();

    // プロジェクション行列の取得
    DirectX::XMMATRIX projectionMatrix = m_camera->GetProjectionMatrix();

    // ビュープロジェクション行列の計算
    DirectX::XMMATRIX viewProjectionMatrix = DirectX::XMMatrixMultiply(viewMatrix, projectionMatrix);

    // 定数バッファに書き込み
    DirectX::XMStoreFloat4x4(&m_constantBufferCPU->world, DirectX::XMMatrixTranspose(worldMatrix));
    DirectX::XMStoreFloat4x4(&m_constantBufferCPU->viewProjection, DirectX::XMMatrixTranspose(viewProjectionMatrix));
    m_constantBufferCPU->cameraPosition = m_camera->GetPosition();
    m_constantBufferCPU->padding = 0.0f;
}

void ModelRenderer::UpdateMaterial(const std::shared_ptr<Material>& material)
{
    if (!m_materialConstantBufferCPU || !material) {
        return;
    }

    // マテリアル定数バッファに書き込み
    m_materialConstantBufferCPU->diffuse = material->diffuse;
    m_materialConstantBufferCPU->ambient = material->ambient;
    m_materialConstantBufferCPU->specular = material->specular;
    m_materialConstantBufferCPU->shininess = material->shininess;
}