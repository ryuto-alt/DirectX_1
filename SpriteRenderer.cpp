#include "SpriteRenderer.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Engine.h"

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <cassert>
#include <algorithm>

SpriteRenderer::SpriteRenderer()
    : m_renderer(nullptr)
    , m_resourceManager(nullptr)
    , m_vertexBufferCPU(nullptr)
    , m_constantBufferCPU(nullptr)
    , m_maxBatchSize(1024)              // 最大1024スプライト
    , m_currentBatchSize(0)
    , m_constantBufferIndex(0)
    , m_batchActive(false)
    , m_initialized(false)
{
}

SpriteRenderer::~SpriteRenderer()
{
    Shutdown();
}

bool SpriteRenderer::Initialize(Renderer* renderer, ResourceManager* resourceManager)
{
    assert(renderer != nullptr);
    assert(resourceManager != nullptr);

    m_renderer = renderer;
    m_resourceManager = resourceManager;

    // パイプラインステートの作成
    if (!CreatePipelineState()) {
        return false;
    }

    // 頂点バッファの作成
    if (!CreateVertexBuffer()) {
        return false;
    }

    // インデックスバッファの作成
    if (!CreateIndexBuffer()) {
        return false;
    }

    // 定数バッファの作成
    if (!CreateConstantBuffer()) {
        return false;
    }

    // ビュープロジェクション行列の更新
    UpdateViewProjection();

    m_initialized = true;
    return true;
}

void SpriteRenderer::Shutdown()
{
    if (m_vertexBufferCPU) {
        m_vertexBuffer->Unmap(0, nullptr);
        m_vertexBufferCPU = nullptr;
    }

    if (m_constantBufferCPU) {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferCPU = nullptr;
    }

    m_initialized = false;
}

bool SpriteRenderer::CreatePipelineState()
{
    // ルートシグネチャの作成
    if (!CreateRootSignature()) {
        return false;
    }

    // コンパイル済みシェーダーの読み込み
    std::shared_ptr<Shader> spriteShader = m_resourceManager->LoadShader(
        L"SpriteShader",
        L"Shaders/SpriteVS.hlsl",
        L"Shaders/SpritePS.hlsl"
    );

    if (!spriteShader || !spriteShader->vertexShaderBlob || !spriteShader->pixelShaderBlob) {
        // シェーダーのコンパイルに失敗した場合は、埋め込みのシェーダーを使用
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        // スプライト用頂点シェーダー
        const char* vertexShaderSource = R"(
            cbuffer SpriteConstantBuffer : register(b0)
            {
                float4x4 ViewProjection;
            };
            
            struct VSInput
            {
                float3 Position : POSITION;
                float2 TexCoord : TEXCOORD;
                float4 Color : COLOR;
            };
            
            struct PSInput
            {
                float4 Position : SV_POSITION;
                float2 TexCoord : TEXCOORD;
                float4 Color : COLOR;
            };
            
            PSInput main(VSInput input)
            {
                PSInput output;
                output.Position = mul(float4(input.Position, 1.0f), ViewProjection);
                output.TexCoord = input.TexCoord;
                output.Color = input.Color;
                return output;
            }
        )";

        // スプライト用ピクセルシェーダー
        const char* pixelShaderSource = R"(
            Texture2D SpriteTexture : register(t0);
            SamplerState SpriteSampler : register(s0);
            
            struct PSInput
            {
                float4 Position : SV_POSITION;
                float2 TexCoord : TEXCOORD;
                float4 Color : COLOR;
            };
            
            float4 main(PSInput input) : SV_TARGET
            {
                float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
                return texColor * input.Color;
            }
        )";

        // 頂点シェーダーのコンパイル
        HRESULT hr = D3DCompile(
            vertexShaderSource,
            strlen(vertexShaderSource),
            "SpriteVS",
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
            "SpritePS",
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
        spriteShader = std::make_shared<Shader>();
        spriteShader->name = L"EmbeddedSpriteShader";
        spriteShader->vertexShaderBlob = vertexShaderBlob;
        spriteShader->pixelShaderBlob = pixelShaderBlob;
    }

    // 入力レイアウトの設定
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(spriteShader->vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(spriteShader->pixelShaderBlob.Get());
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

bool SpriteRenderer::CreateRootSignature()
{
    // ルートパラメータの設定
    CD3DX12_ROOT_PARAMETER rootParameters[1];

    // 定数バッファビュー（b0）
    CD3DX12_DESCRIPTOR_RANGE cbvRange;
    cbvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    rootParameters[0].InitAsDescriptorTable(1, &cbvRange);

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

bool SpriteRenderer::CreateVertexBuffer()
{
    // 頂点バッファのサイズを計算
    const UINT vertexBufferSize = m_maxBatchSize * 4 * sizeof(SpriteVertex);

    // 頂点バッファの作成
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    HRESULT hr = m_renderer->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)
    );

    if (FAILED(hr)) {
        return false;
    }

    // 頂点バッファビューの設定
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(SpriteVertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    // 頂点バッファをマップ
    CD3DX12_RANGE readRange(0, 0);
    hr = m_vertexBuffer->Map(
        0,
        &readRange,
        reinterpret_cast<void**>(&m_vertexBufferCPU)
    );

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool SpriteRenderer::CreateIndexBuffer()
{
    // 各スプライトは2つの三角形（6つのインデックス）を使用
    const UINT indexBufferSize = m_maxBatchSize * 6 * sizeof(UINT);
    std::vector<UINT> indices(m_maxBatchSize * 6);

    // インデックスバッファのデータを設定
    for (int i = 0; i < m_maxBatchSize; ++i) {
        // 各スプライトの頂点インデックス（4頂点）
        int vertexOffset = i * 4;

        // 各スプライトのインデックスオフセット（6インデックス）
        int indexOffset = i * 6;

        // 最初の三角形
        indices[indexOffset + 0] = vertexOffset + 0;
        indices[indexOffset + 1] = vertexOffset + 1;
        indices[indexOffset + 2] = vertexOffset + 2;

        // 2番目の三角形
        indices[indexOffset + 3] = vertexOffset + 0;
        indices[indexOffset + 4] = vertexOffset + 2;
        indices[indexOffset + 5] = vertexOffset + 3;
    }

    // インデックスバッファの作成
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    HRESULT hr = m_renderer->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_indexBuffer)
    );

    if (FAILED(hr)) {
        return false;
    }

    // インデックスバッファビューの設定
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;

    // インデックスバッファにデータをコピー
    UINT8* indexBufferCPU = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexBufferCPU));
    if (FAILED(hr)) {
        return false;
    }

    memcpy(indexBufferCPU, indices.data(), indexBufferSize);

    m_indexBuffer->Unmap(0, nullptr);

    return true;
}

bool SpriteRenderer::CreateConstantBuffer()
{
    // 定数バッファのサイズを計算（256バイトアラインメント）
    const UINT constantBufferSize = (sizeof(SpriteConstantBuffer) + 255) & ~255;

    // 定数バッファの作成
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

    // 定数バッファをマップ
    CD3DX12_RANGE readRange(0, 0);
    hr = m_constantBuffer->Map(
        0,
        &readRange,
        reinterpret_cast<void**>(&m_constantBufferCPU)
    );

    if (FAILED(hr)) {
        return false;
    }

    // CBV/SRV/UAVヒープにCBVを作成
    m_constantBufferIndex = 0; // エンジンから適切なインデックスを取得する必要がある

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

    return true;
}

void SpriteRenderer::UpdateViewProjection()
{
    if (!m_constantBufferCPU) {
        return;
    }

    // オーソグラフィック（正射影）ビュープロジェクション行列の作成
    int width = m_renderer->GetViewport().Width;
    int height = m_renderer->GetViewport().Height;

    // ビュー行列の作成（単位行列）
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();

    // プロジェクション行列の作成（正射影）
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f,                                           // 左端
        static_cast<float>(width),                      // 右端
        static_cast<float>(height),                     // 下端
        0.0f,                                           // 上端
        0.0f,                                           // 近
        1.0f                                            // 遠
    );

    // ビュープロジェクション行列の計算
    DirectX::XMMATRIX viewProjection = DirectX::XMMatrixMultiply(view, projection);

    // 定数バッファに書き込み
    DirectX::XMStoreFloat4x4(&m_constantBufferCPU->viewProjection, DirectX::XMMatrixTranspose(viewProjection));
}

std::shared_ptr<Sprite> SpriteRenderer::CreateSprite(const std::wstring& texturePath)
{
    // テクスチャの読み込み
    std::shared_ptr<Texture> texture = m_resourceManager->LoadTexture(texturePath);
    if (!texture) {
        return nullptr;
    }

    // スプライトの作成
    std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>();
    sprite->texture = texture;
    sprite->position = DirectX::XMFLOAT2(0.0f, 0.0f);
    sprite->size = DirectX::XMFLOAT2(
        static_cast<float>(texture->metadata.width),
        static_cast<float>(texture->metadata.height)
    );
    sprite->origin = DirectX::XMFLOAT2(0.5f, 0.5f);  // デフォルトは中心
    sprite->rotation = 0.0f;
    sprite->color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 白色
    sprite->texCoordTL = DirectX::XMFLOAT2(0.0f, 0.0f);
    sprite->texCoordBR = DirectX::XMFLOAT2(1.0f, 1.0f);
    sprite->depth = 0.0f;
    sprite->visible = true;

    return sprite;
}

void SpriteRenderer::Begin()
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

    // 頂点バッファとインデックスバッファの設定
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // ディスクリプタヒープの設定
    ID3D12DescriptorHeap* heaps[] = { m_resourceManager->GetCbvSrvUavHeap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // 定数バッファビューの設定
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(
        m_resourceManager->GetCbvSrvUavHeap()->GetGPUDescriptorHandleForHeapStart(),
        m_constantBufferIndex,
        m_renderer->GetCbvSrvUavDescriptorSize()
    );

    commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

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

    // バッチサイズをリセット
    m_currentBatchSize = 0;

    // バッチアクティブフラグを設定
    m_batchActive = true;
}

void SpriteRenderer::Draw(const std::shared_ptr<Sprite>& sprite)
{
    if (!m_initialized || !m_batchActive || !sprite || !sprite->visible) {
        return;
    }

    // バッチが一杯の場合は描画して新しいバッチを開始
    if (m_currentBatchSize >= m_maxBatchSize) {
        End();
        Begin();
    }

    // スプライトの頂点データを設定
    SpriteVertex* vertices = m_vertexBufferCPU + (m_currentBatchSize * 4);
    SetSpriteVertices(sprite, vertices);

    // スプライト数を増やす
    m_currentBatchSize++;
}

void SpriteRenderer::End()
{
    if (!m_initialized || !m_batchActive || m_currentBatchSize == 0) {
        m_batchActive = false;
        return;
    }

    // 描画コマンドの発行
    m_renderer->GetCommandList()->DrawIndexedInstanced(
        m_currentBatchSize * 6,  // インデックス数
        1,                       // インスタンス数
        0,                       // 開始インデックス
        0,                       // 頂点オフセット
        0                        // インスタンスオフセット
    );

    // バッチアクティブフラグをリセット
    m_batchActive = false;
}

void SpriteRenderer::SetSpriteVertices(const std::shared_ptr<Sprite>& sprite, SpriteVertex* vertices)
{
    // スプライトの中心位置
    float centerX = sprite->position.x;
    float centerY = sprite->position.y;

    // 原点オフセット
    float originX = sprite->size.x * sprite->origin.x;
    float originY = sprite->size.y * sprite->origin.y;

    // スプライトの四隅の位置（回転前）
    float left = -originX;
    float right = sprite->size.x - originX;
    float top = -originY;
    float bottom = sprite->size.y - originY;

    // 回転行列の計算
    float cosRotation = cosf(sprite->rotation);
    float sinRotation = sinf(sprite->rotation);

    // 左上頂点
    vertices[0].position = DirectX::XMFLOAT3(
        centerX + left * cosRotation - top * sinRotation,
        centerY + left * sinRotation + top * cosRotation,
        sprite->depth
    );
    vertices[0].texCoord = DirectX::XMFLOAT2(sprite->texCoordTL.x, sprite->texCoordTL.y);
    vertices[0].color = sprite->color;

    // 左下頂点
    vertices[1].position = DirectX::XMFLOAT3(
        centerX + left * cosRotation - bottom * sinRotation,
        centerY + left * sinRotation + bottom * cosRotation,
        sprite->depth
    );
    vertices[1].texCoord = DirectX::XMFLOAT2(sprite->texCoordTL.x, sprite->texCoordBR.y);
    vertices[1].color = sprite->color;

    // 右下頂点
    vertices[2].position = DirectX::XMFLOAT3(
        centerX + right * cosRotation - bottom * sinRotation,
        centerY + right * sinRotation + bottom * cosRotation,
        sprite->depth
    );
    vertices[2].texCoord = DirectX::XMFLOAT2(sprite->texCoordBR.x, sprite->texCoordBR.y);
    vertices[2].color = sprite->color;

    // 右上頂点
    vertices[3].position = DirectX::XMFLOAT3(
        centerX + right * cosRotation - top * sinRotation,
        centerY + right * sinRotation + top * cosRotation,
        sprite->depth
    );
    vertices[3].texCoord = DirectX::XMFLOAT2(sprite->texCoordBR.x, sprite->texCoordTL.y);
    vertices[3].color = sprite->color;
}