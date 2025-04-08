#include "Renderer.h"
#include "Window.h"

#include <directx/d3dx12.h>
#include <dxgidebug.h>
#include <cassert>
#include <stdexcept>

Renderer::Renderer()
    : m_currentBackBuffer(0)
    , m_backBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    , m_depthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
{
    // インクリメントサイズの初期化
    m_rtvDescriptorSize = 0;
    m_dsvDescriptorSize = 0;
    m_cbvSrvUavDescriptorSize = 0;

    // フェンス値の初期化
    for (UINT i = 0; i < kBackBufferCount; ++i) {
        m_fenceValues[i] = 0;
    }

    // ビューポートとシザー矩形の初期化
    m_viewport = {};
    m_scissorRect = {};
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize(Window* window)
{
    assert(window != nullptr);

    // ウィンドウサイズの取得
    m_width = window->GetWidth();
    m_height = window->GetHeight();

    // デバイスとスワップチェインの作成
    if (!CreateDeviceAndSwapChain(window)) {
        return false;
    }

    // コマンドオブジェクトの作成
    if (!CreateCommandObjects()) {
        return false;
    }

    // ディスクリプタヒープの作成
    if (!CreateDescriptorHeaps()) {
        return false;
    }

    // レンダーターゲットビューの作成
    if (!CreateRenderTargetViews()) {
        return false;
    }

    // 深度ステンシルビューの作成
    if (!CreateDepthStencilView()) {
        return false;
    }

    // フェンスの作成
    if (!CreateFence()) {
        return false;
    }

    // ビューポートの設定
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(m_width);
    m_viewport.Height = static_cast<float>(m_height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    // シザー矩形の設定
    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = m_width;
    m_scissorRect.bottom = m_height;

    m_initialized = true;
    return true;
}

void Renderer::Shutdown()
{
    // GPUの処理完了を待機
    WaitForGpu();

    // フェンスイベントを閉じる
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    m_initialized = false;
}

bool Renderer::CreateDeviceAndSwapChain(Window* window)
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // デバッグレイヤーを有効化
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            // DXGIデバッグも有効化
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    // DXGIファクトリーの作成
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr)) {
        return false;
    }

    // ハードウェアアダプタの検索
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // ソフトウェアアダプタはスキップ
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        // このアダプタでD3D12デバイスの作成を試みる
        hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
        if (SUCCEEDED(hr)) {
            break;
        }
    }

    // アダプタが見つからなかった場合はWARPデバイスを使用
    if (!m_device) {
        Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
        m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

        hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
        if (FAILED(hr)) {
            return false;
        }
    }

    // コマンドキューの作成
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        return false;
    }

    // スワップチェインの作成
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = kBackBufferCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = m_backBufferFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
    hr = m_dxgiFactory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        window->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1
    );

    if (FAILED(hr)) {
        return false;
    }

    // IDXGISwapChain4への変換
    hr = swapChain1.As(&m_swapChain);
    if (FAILED(hr)) {
        return false;
    }

    // ALT+ENTERでのフルスクリーン切り替えを無効化（ウィンドウクラスで独自に処理）
    m_dxgiFactory->MakeWindowAssociation(window->GetHwnd(), DXGI_MWA_NO_ALT_ENTER);

    // 現在のバックバッファインデックスを設定
    m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();

    return true;
}

bool Renderer::CreateCommandObjects()
{
    // コマンドアロケータの作成
    for (UINT i = 0; i < kBackBufferCount; ++i) {
        HRESULT hr = m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_commandAllocators[i])
        );

        if (FAILED(hr)) {
            return false;
        }
    }

    // コマンドリストの作成
    HRESULT hr = m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[m_currentBackBuffer].Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList)
    );

    if (FAILED(hr)) {
        return false;
    }

    // コマンドリストは最初は閉じた状態にしておく
    m_commandList->Close();

    return true;
}

bool Renderer::CreateDescriptorHeaps()
{
    // ディスクリプタサイズの取得
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // RTVヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = kBackBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr)) {
        return false;
    }

    // DSVヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
    if (FAILED(hr)) {
        return false;
    }

    // CBV/SRV/UAVヒープの作成（シェーダーから見えるデスクリプタ）
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
    cbvSrvUavHeapDesc.NumDescriptors = 1000; // 必要に応じて調整
    cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap));
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool Renderer::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    // 各バックバッファに対してRTVを作成
    for (UINT i = 0; i < kBackBufferCount; ++i) {
        // バックバッファの取得
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
        if (FAILED(hr)) {
            return false;
        }

        // RTVの作成
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHeapHandle);

        // 次のディスクリプタへ
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
    }

    return true;
}

bool Renderer::CreateDepthStencilView()
{
    // 深度ステンシルバッファの作成
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = m_depthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = m_depthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_depthStencilBuffer)
    );

    if (FAILED(hr)) {
        return false;
    }

    // 深度ステンシルビューの作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = m_depthStencilFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    m_device->CreateDepthStencilView(
        m_depthStencilBuffer.Get(),
        &dsvDesc,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    return true;
}

bool Renderer::CreateFence()
{
    // フェンスの作成
    HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) {
        return false;
    }

    // フェンスイベントの作成
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        return false;
    }

    // フェンス値の初期化
    for (UINT i = 0; i < kBackBufferCount; ++i) {
        m_fenceValues[i] = 0;
    }

    return true;
}

void Renderer::BeginFrame()
{
    // コマンドアロケータのリセット
    m_commandAllocators[m_currentBackBuffer]->Reset();

    // コマンドリストのリセット
    m_commandList->Reset(m_commandAllocators[m_currentBackBuffer].Get(), nullptr);

    // バックバッファをレンダーターゲットとして使用するための状態遷移
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_currentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_commandList->ResourceBarrier(1, &barrier);

    // ビューポートとシザー矩形の設定
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // バックバッファのクリア
    ClearBackBuffer();

    // 深度バッファのクリア
    ClearDepthStencilBuffer();
}

void Renderer::EndFrame()
{
    // バックバッファを表示用の状態に戻す
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_currentBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    m_commandList->ResourceBarrier(1, &barrier);

    // コマンドリストのクローズ
    m_commandList->Close();

    // コマンドリストの実行
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // バックバッファの表示
    HRESULT hr = m_swapChain->Present(1, 0);
    if (FAILED(hr)) {
        // エラー処理
        return;
    }

    // GPUの処理完了を待機
    m_fenceValues[m_currentBackBuffer]++;
    const UINT64 fenceValue = m_fenceValues[m_currentBackBuffer];

    // シグナルをキューに追加
    hr = m_commandQueue->Signal(m_fence.Get(), fenceValue);
    if (FAILED(hr)) {
        // エラー処理
        return;
    }

    // 次のバックバッファに切り替え
    m_currentBackBuffer = m_swapChain->GetCurrentBackBufferIndex();

    // 前のフレームのGPU処理が完了するまで待機
    if (m_fence->GetCompletedValue() < m_fenceValues[m_currentBackBuffer]) {
        hr = m_fence->SetEventOnCompletion(m_fenceValues[m_currentBackBuffer], m_fenceEvent);
        if (SUCCEEDED(hr)) {
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
}

void Renderer::ClearBackBuffer()
{
    // レンダーターゲットビューのハンドルを取得
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_currentBackBuffer,
        m_rtvDescriptorSize
    );

    // クリアカラー（RGBA）
    const float clearColor[] = { 0.0f, 0.0f, 0.2f, 1.0f }; // 濃紺色

    // バックバッファのクリア
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Renderer::ClearDepthStencilBuffer()
{
    // 深度ステンシルビューのハンドルを取得
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // 深度ステンシルバッファのクリア
    m_commandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,  // 深度値
        0,     // ステンシル値
        0,
        nullptr
    );
}

void Renderer::OnResize(int width, int height)
{
    assert(m_device);
    assert(m_swapChain);
    assert(m_commandList);

    // 新しいサイズが有効かチェック
    if (width <= 0 || height <= 0) {
        return;
    }

    // サイズが変わらない場合は何もしない
    if (m_width == width && m_height == height) {
        return;
    }

    // 既存のGPU処理が完了するまで待機
    WaitForGpu();

    // コマンドリストをリセット
    m_commandList->Reset(m_commandAllocators[m_currentBackBuffer].Get(), nullptr);

    // レンダーターゲットビューを解放
    for (UINT i = 0; i < kBackBufferCount; ++i) {
        m_renderTargets[i].Reset();
    }

    // 深度ステンシルバッファを解放
    m_depthStencilBuffer.Reset();

    // スワップチェインのリサイズ
    HRESULT hr = m_swapChain->ResizeBuffers(
        kBackBufferCount,
        width,
        height,
        m_backBufferFormat,
        0
    );

    if (FAILED(hr)) {
        // エラー処理
        return;
    }

    // 新しいサイズを保存
    m_width = width;
    m_height = height;

    // 現在のバックバッファインデックスをリセット
    m_currentBackBuffer = 0;

    // 新しいレンダーターゲットビューを作成
    CreateRenderTargetViews();

    // 新しい深度ステンシルビューを作成
    CreateDepthStencilView();

    // ビューポートとシザー矩形の更新
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);

    m_scissorRect.right = width;
    m_scissorRect.bottom = height;

    // コマンドリストを閉じて実行
    m_commandList->Close();
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // GPUの処理が完了するのを待機
    WaitForGpu();
}

void Renderer::ExecuteCommandList()
{
    // コマンドリストのクローズ
    m_commandList->Close();

    // コマンドリストの実行
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // GPUの処理完了を待機
    WaitForGpu();

    // コマンドリストのリセット
    m_commandList->Reset(m_commandAllocators[m_currentBackBuffer].Get(), nullptr);
}

void Renderer::WaitForGpu()
{
    if (!m_device || !m_fence || !m_commandQueue) {
        return;
    }

    // 現在のフェンス値を増加
    const UINT64 fenceValue = m_fenceValues[m_currentBackBuffer];

    // シグナルをキューに追加
    HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fenceValue);
    if (FAILED(hr)) {
        // エラー処理
        return;
    }

    // フェンスの完了イベントを設定
    if (m_fence->GetCompletedValue() < fenceValue) {
        hr = m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        if (SUCCEEDED(hr)) {
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
}