#include "DXMesh.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

DXMesh::DXMesh(ID3D12Device* device)
    : m_device(device), m_vertexCount(0), m_indexCount(0)
{
}

DXMesh::~DXMesh()
{
    // Resources are automatically cleaned up by ComPtr
}

bool DXMesh::CreateQuad()
{
    // Define vertices for a quad (rectangle)
    Vertex vertices[] =
    {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) }, // Bottom-left
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) }, // Top-left
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }, // Bottom-right
        { XMFLOAT3(1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }  // Top-right
    };

    // Define indices for two triangles that form the quad
    UINT16 indices[] = {
        0, 1, 2, // First triangle
        2, 1, 3  // Second triangle
    };

    m_vertexCount = _countof(vertices);
    m_indexCount = _countof(indices);

    // Create vertex buffer
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeof(vertices);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer));

    if (FAILED(hr))
        return false;

    // Copy vertex data to the vertex buffer
    void* pVertexData = nullptr;
    hr = m_vertexBuffer->Map(0, nullptr, &pVertexData);
    if (FAILED(hr))
        return false;

    memcpy(pVertexData, vertices, sizeof(vertices));
    m_vertexBuffer->Unmap(0, nullptr);

    // Create vertex buffer view
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = sizeof(vertices);

    // Create index buffer
    resourceDesc.Width = sizeof(indices);

    hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_indexBuffer));

    if (FAILED(hr))
        return false;

    // Copy index data to the index buffer
    void* pIndexData = nullptr;
    hr = m_indexBuffer->Map(0, nullptr, &pIndexData);
    if (FAILED(hr))
        return false;

    memcpy(pIndexData, indices, sizeof(indices));
    m_indexBuffer->Unmap(0, nullptr);

    // Create index buffer view
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = sizeof(indices);

    return true;
}