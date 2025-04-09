#include "DXRenderer.h"
#include "DXShader.h"
#include "DXTexture.h"
#include "DXMesh.h"
#include "DXCamera.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <iostream>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace Microsoft::WRL;
using namespace DirectX;

DXRenderer::DXRenderer(HWND hwnd, int width, int height)
    : m_hwnd(hwnd), m_width(width), m_height(height), m_fenceValue(0),
    m_currentBackBuffer(0), m_angle(0.0f), m_shader(nullptr),
    m_texture(nullptr), m_mesh(nullptr), m_camera(nullptr),
    m_constantBufferMappedData(nullptr)
{
    m_worldMatrix = XMMatrixIdentity();
}

DXRenderer::~DXRenderer()
{
    // Wait for GPU to complete all work before destroying resources
    WaitForGpu();

    // Clean up heap-allocated objects
    if (m_shader) {
        delete m_shader;
        m_shader = nullptr;
    }

    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }

    if (m_mesh) {
        delete m_mesh;
        m_mesh = nullptr;
    }

    if (m_camera) {
        delete m_camera;
        m_camera = nullptr;
    }

    // Unmap constant buffer if it's mapped
    if (m_constantBuffer && m_constantBufferMappedData) {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferMappedData = nullptr;
    }
}

bool DXRenderer::Initialize()
{
#ifdef _DEBUG
    // Enable debug layer in debug mode
    EnableDebugLayer();
#endif

    // Initialize DirectX components
    if (!InitializeDevice()) return false;
    if (!InitializeCommandQueue()) return false;
    if (!InitializeSwapChain()) return false;
    if (!InitializeRenderTargets()) return false;
    if (!InitializeRootSignature()) return false;
    if (!InitializePipelineState()) return false;
    if (!InitializeViewport()) return false;
    if (!InitializeScissorRect()) return false;
    if (!InitializeAssets()) return false;

    return true;
}

void DXRenderer::EnableDebugLayer()
{
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif
}

bool DXRenderer::InitializeDevice()
{
    UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    // Create DXGI factory
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create DXGI factory" << std::endl;
        return false;
    }

    // Find the best adapter (GPU)
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; SUCCEEDED(m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        // Try to create the device with this adapter
        hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (SUCCEEDED(hr))
            break;
    }

    if (!m_device)
    {
        // If no hardware adapter found, create a WARP device
        ComPtr<IDXGIAdapter> warpAdapter;
        hr = m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        if (FAILED(hr))
        {
            std::cerr << "Failed to find WARP adapter" << std::endl;
            return false;
        }

        hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (FAILED(hr))
        {
            std::cerr << "Failed to create WARP device" << std::endl;
            return false;
        }
    }

    return true;
}

bool DXRenderer::InitializeCommandQueue()
{
    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_cmdQueue));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create command queue" << std::endl;
        return false;
    }

    // Create command allocator
    hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create command allocator" << std::endl;
        return false;
    }

    // Create command list
    hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_cmdList));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create command list" << std::endl;
        return false;
    }

    // Close the command list to reset it later
    hr = m_cmdList->Close();
    if (FAILED(hr))
    {
        std::cerr << "Failed to close command list" << std::endl;
        return false;
    }

    // Create fence
    hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create fence" << std::endl;
        return false;
    }

    return true;
}

bool DXRenderer::InitializeSwapChain()
{
    // Release any existing swap chain
    m_swapChain.Reset();

    // Create swap chain descriptor
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Create the swap chain
    ComPtr<IDXGISwapChain1> swapChain1;
    HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
        m_cmdQueue.Get(),
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1);

    if (FAILED(hr))
    {
        std::cerr << "Failed to create swap chain" << std::endl;
        return false;
    }

    // Get the SwapChain4 interface
    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr))
    {
        std::cerr << "Failed to query SwapChain4 interface" << std::endl;
        return false;
    }

    // Disable alt+enter fullscreen toggle
    hr = m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr))
    {
        std::cerr << "Failed to disable alt+enter fullscreen toggle" << std::endl;
        return false;
    }

    m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DXRenderer::InitializeRenderTargets()
{
    // Create descriptor heap for render target views
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 2; // Double buffering
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create RTV descriptor heap" << std::endl;
        return false;
    }

    // Create descriptor heap for shader resource views
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 2; // Texture + constant buffer
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_basicDescHeap));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create CBV/SRV/UAV descriptor heap" << std::endl;
        return false;
    }

    // Get descriptor handle size
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create render target views
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    m_renderTargets.resize(2);

    // Create a RTV for each frame resource
    for (UINT i = 0; i < 2; i++)
    {
        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
        if (FAILED(hr))
        {
            std::cerr << "Failed to get swap chain buffer " << i << std::endl;
            return false;
        }

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), &rtvDesc, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }

    return true;
}

bool DXRenderer::InitializeRootSignature()
{
    // Create descriptor range for textures
    D3D12_DESCRIPTOR_RANGE descRanges[2] = {};

    // Texture SRV
    descRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRanges[0].NumDescriptors = 1;
    descRanges[0].BaseShaderRegister = 0;
    descRanges[0].RegisterSpace = 0;
    descRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Constant buffer view
    descRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descRanges[1].NumDescriptors = 1;
    descRanges[1].BaseShaderRegister = 0;
    descRanges[1].RegisterSpace = 0;
    descRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Create root parameter
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 2;
    rootParam.DescriptorTable.pDescriptorRanges = descRanges;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Create static sampler
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Create root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParam;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &sampler;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Serialize root signature
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            std::cerr << "Failed to serialize root signature: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
        }
        else
        {
            std::cerr << "Failed to serialize root signature" << std::endl;
        }
        return false;
    }

    // Create root signature
    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create root signature" << std::endl;
        return false;
    }

    return true;
}

bool DXRenderer::InitializePipelineState()
{
    // Create the shader objects first
    m_shader = new DXShader(m_device.Get());
    if (!m_shader->CompileVertexShader(L"./BasicVertexShader.hlsl", "BasicVS"))
    {
        std::cerr << "Failed to compile vertex shader" << std::endl;
        return false;
    }

    if (!m_shader->CompilePixelShader(L"./BasicPixelShader.hlsl", "BasicPS"))
    {
        std::cerr << "Failed to compile pixel shader" << std::endl;
        return false;
    }

    // Define input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Create pipeline state description
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_shader->GetVertexShaderBlob());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_shader->GetPixelShaderBlob());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;

    // Create the pipeline state object
    HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create pipeline state object" << std::endl;
        return false;
    }

    return true;
}

bool DXRenderer::InitializeViewport()
{
    // Setup viewport
    m_viewport.Width = static_cast<float>(m_width);
    m_viewport.Height = static_cast<float>(m_height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;

    return true;
}

bool DXRenderer::InitializeScissorRect()
{
    // Setup scissor rect
    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = m_width;
    m_scissorRect.bottom = m_height;

    return true;
}

bool DXRenderer::InitializeAssets()
{
    // Reset command allocator and list for initialization commands
    HRESULT hr = m_cmdAllocator->Reset();
    if (FAILED(hr))
    {
        std::cerr << "Failed to reset command allocator" << std::endl;
        return false;
    }

    hr = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
    if (FAILED(hr))
    {
        std::cerr << "Failed to reset command list" << std::endl;
        return false;
    }

    // Create mesh
    m_mesh = new DXMesh(m_device.Get());
    if (!m_mesh->CreateQuad())
    {
        std::cerr << "Failed to create mesh" << std::endl;
        return false;
    }

    // Create camera with appropriate aspect ratio
    float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    m_camera = new DXCamera(XM_PIDIV2, aspectRatio, 1.0f, 100.0f);
    m_camera->SetPosition(XMFLOAT3(0.0f, 0.0f, -5.0f));
    m_camera->SetTarget(XMFLOAT3(0.0f, 0.0f, 0.0f));
    m_camera->SetUp(XMFLOAT3(0.0f, 1.0f, 0.0f));

    // Create constant buffer
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 255) & ~255);

    hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer));

    if (FAILED(hr))
    {
        std::cerr << "Failed to create constant buffer" << std::endl;
        return false;
    }

    // Map constant buffer
    hr = m_constantBuffer->Map(0, nullptr, &m_constantBufferMappedData);
    if (FAILED(hr))
    {
        std::cerr << "Failed to map constant buffer" << std::endl;
        return false;
    }

    // Initialize with identity matrix
    memcpy(m_constantBufferMappedData, &m_worldMatrix, sizeof(XMMATRIX));

    // Create texture
    m_texture = new DXTexture(m_device.Get(), m_cmdList.Get());
    if (!m_texture->LoadFromFile(L"./img/uvChecker.png"))
    {
        std::cerr << "Failed to load texture" << std::endl;
        return false;
    }

    // Create shader resource view for texture
    auto srvDesc = m_texture->GetSrvDesc();
    m_device->CreateShaderResourceView(
        m_texture->GetResource(),
        &srvDesc,
        m_basicDescHeap->GetCPUDescriptorHandleForHeapStart()
    );


    // Create constant buffer view
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = (sizeof(XMMATRIX) + 255) & ~255;

    // Create constant buffer view at the second position in the descriptor heap
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_basicDescHeap->GetCPUDescriptorHandleForHeapStart());
    cbvHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // Execute init commands
    m_cmdList->Close();
    ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Wait for initialization to complete
    WaitForGpu();

    return true;
}

void DXRenderer::Render()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU
    HRESULT hr = m_cmdAllocator->Reset();
    if (FAILED(hr))
    {
        std::cerr << "Failed to reset command allocator" << std::endl;
        return;
    }

    // Reset the command list
    hr = m_cmdList->Reset(m_cmdAllocator.Get(), m_pipelineState.Get());
    if (FAILED(hr))
    {
        std::cerr << "Failed to reset command list" << std::endl;
        return;
    }

    // Get current back buffer index
    m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();

    // Transition back buffer from present to render target
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_currentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_cmdList->ResourceBarrier(1, &barrier);

    // Set necessary state for rendering
    m_cmdList->SetPipelineState(m_pipelineState.Get());
    m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_basicDescHeap.Get() };
    m_cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    m_cmdList->SetGraphicsRootDescriptorTable(0, m_basicDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Set viewport and scissor rect
    m_cmdList->RSSetViewports(1, &m_viewport);
    m_cmdList->RSSetScissorRects(1, &m_scissorRect);

    // Set render target
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_currentBackBuffer,
        m_rtvDescriptorSize
    );
    m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Clear render target
    const float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    m_cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // 単純にY軸回転を更新
    m_angle += 0.01f;
    m_worldMatrix = XMMatrixRotationY(m_angle);

    // ワールド・ビュー・プロジェクション行列を合成
    XMMATRIX combinedMatrix = m_worldMatrix * m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix();

    // 定数バッファに書き込み
    memcpy(m_constantBufferMappedData, &combinedMatrix, sizeof(XMMATRIX));

    // Draw
    auto vertexBufferView = m_mesh->GetVertexBufferView();
    auto indexBufferView = m_mesh->GetIndexBufferView();
    m_cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
    m_cmdList->IASetIndexBuffer(&indexBufferView);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawIndexedInstanced(m_mesh->GetIndexCount(), 1, 0, 0, 0);

    // Transition back buffer from render target to present
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_currentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    m_cmdList->ResourceBarrier(1, &barrier);

    // Close the command list
    hr = m_cmdList->Close();
    if (FAILED(hr))
    {
        std::cerr << "Failed to close command list" << std::endl;
        return;
    }

    // Execute the command list
    ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
    m_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Present the frame
    hr = m_swapChain->Present(1, 0);
    if (FAILED(hr))
    {
        std::cerr << "Failed to present frame" << std::endl;
        return;
    }

    // Wait for the frame to be rendered
    WaitForGpu();
}

void DXRenderer::WaitForGpu()
{
    // Schedule a signal command
    HRESULT hr = m_cmdQueue->Signal(m_fence.Get(), ++m_fenceValue);
    if (FAILED(hr))
    {
        std::cerr << "Failed to signal fence" << std::endl;
        return;
    }

    // Wait until the fence has been reached
    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr)
        {
            std::cerr << "Failed to create fence event" << std::endl;
            return;
        }

        // Fire event when GPU hits current fence value
        hr = m_fence->SetEventOnCompletion(m_fenceValue, eventHandle);
        if (FAILED(hr))
        {
            std::cerr << "Failed to set fence event" << std::endl;
            CloseHandle(eventHandle);
            return;
        }

        // Wait until the fence has been processed
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

size_t DXRenderer::AlignmentedSize(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}