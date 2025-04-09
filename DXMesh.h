#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 TexCoord;
};

class DXMesh {
public:
    DXMesh(ID3D12Device* device);
    ~DXMesh();

    bool CreateQuad();

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_indexBufferView; }
    UINT GetIndexCount() const { return m_indexCount; }

private:
    ID3D12Device* m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    UINT m_vertexCount;
    UINT m_indexCount;
};