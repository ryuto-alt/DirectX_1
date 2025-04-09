#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

class DXShader;
class DXTexture;
class DXMesh;
class DXCamera;

class DXRenderer {
public:
    DXRenderer(HWND hwnd, int width, int height);
    ~DXRenderer();

    bool Initialize();
    void Render();
    void WaitForGpu();

private:
    bool InitializeDevice();
    bool InitializeCommandQueue();
    bool InitializeSwapChain();
    bool InitializeRenderTargets();
    bool InitializeRootSignature();
    bool InitializePipelineState();
    bool InitializeViewport();
    bool InitializeScissorRect();
    bool InitializeAssets();
    void EnableDebugLayer();
    size_t AlignmentedSize(size_t size, size_t alignment);

    HWND m_hwnd;
    int m_width;
    int m_height;

    // DirectX Resources
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_basicDescHeap;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_renderTargets;

    UINT m_rtvDescriptorSize;
    UINT m_currentBackBuffer;
    UINT64 m_fenceValue;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    // Helper Classes
    DXShader* m_shader;
    DXTexture* m_texture;
    DXMesh* m_mesh;
    DXCamera* m_camera;

    // Matrix and Constant Buffer
    DirectX::XMMATRIX m_worldMatrix;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferMappedData;

    float m_angle;
};