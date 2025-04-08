#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include <DirectXMath.h>
#include <memory>

class Window;

// レンダリングを管理するクラス
class Renderer {
public:
    Renderer();
    ~Renderer();

    // レンダラーの初期化
    bool Initialize(Window* window);

    // レンダラーの終了処理
    void Shutdown();

    // フレームの開始
    void BeginFrame();

    // フレームの終了
    void EndFrame();

    // ウィンドウのリサイズ時の処理
    void OnResize(int width, int height);

    // デバイスの取得
    ID3D12Device* GetDevice() const { return m_device.Get(); }

    // コマンドキューの取得
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }

    // コマンドリストの取得
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    // レンダーターゲットビューのヒープを取得
    ID3D12DescriptorHeap* GetRtvHeap() const { return m_rtvHeap.Get(); }

    // 深度ステンシルビューのヒープを取得
    ID3D12DescriptorHeap* GetDsvHeap() const { return m_dsvHeap.Get(); }

    // CBV/SRV/UAVヒープを取得
    ID3D12DescriptorHeap* GetCbvSrvUavHeap() const { return m_cbvSrvUavHeap.Get(); }

    // バックバッファのフォーマットを取得
    DXGI_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }

    // 深度バッファのフォーマットを取得
    DXGI_FORMAT GetDepthStencilFormat() const { return m_depthStencilFormat; }

    // 現在のビューポートを取得
    const D3D12_VIEWPORT& GetViewport() const { return m_viewport; }

    // 現在のシザー矩形を取得
    const D3D12_RECT& GetScissorRect() const { return m_scissorRect; }

    // CBV/SRV/UAVディスクリプタのインクリメントサイズを取得
    UINT GetCbvSrvUavDescriptorSize() const { return m_cbvSrvUavDescriptorSize; }

    // RTVディスクリプタのインクリメントサイズを取得
    UINT GetRtvDescriptorSize() const { return m_rtvDescriptorSize; }

    // DSVディスクリプタのインクリメントサイズを取得
    UINT GetDsvDescriptorSize() const { return m_dsvDescriptorSize; }

    // バックバッファ数を取得
    static const UINT GetBackBufferCount() { return kBackBufferCount; }

    // コマンドリストを実行して完了を待機
    void ExecuteCommandList();

    // GPUの処理完了を待機
    void WaitForGpu();

private:
    // DirectX 12デバイスとスワップチェインの作成
    bool CreateDeviceAndSwapChain(Window* window);

    // コマンドオブジェクトの作成
    bool CreateCommandObjects();

    // ディスクリプタヒープの作成
    bool CreateDescriptorHeaps();

    // レンダーターゲットビューの作成
    bool CreateRenderTargetViews();

    // 深度ステンシルビューの作成
    bool CreateDepthStencilView();

    // フェンスの作成
    bool CreateFence();

    // バックバッファのクリア
    void ClearBackBuffer();

    // 深度バッファのクリア
    void ClearDepthStencilBuffer();

    // 定数
    static const UINT kBackBufferCount = 2;  // ダブルバッファリング

    // デバイスとスワップチェイン
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;

    // コマンドオブジェクト
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[kBackBufferCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // ディスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;             // レンダーターゲットビューのヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;             // 深度ステンシルビューのヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;       // CBV/SRV/UAVのヒープ

    // インクリメントサイズ
    UINT m_rtvDescriptorSize;
    UINT m_dsvDescriptorSize;
    UINT m_cbvSrvUavDescriptorSize;

    // バックバッファとレンダーターゲットビュー
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[kBackBufferCount];
    UINT m_currentBackBuffer;

    // 深度ステンシルバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

    // 同期オブジェクト
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[kBackBufferCount];
    HANDLE m_fenceEvent;

    // フォーマット
    DXGI_FORMAT m_backBufferFormat;
    DXGI_FORMAT m_depthStencilFormat;

    // ビューポートとシザー矩形
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    // ウィンドウサイズ
    int m_width;
    int m_height;

    // 初期化フラグ
    bool m_initialized;
};