#include "Model.h"
#include "Material.h"
#include "Skeleton.h"
#include "Animation.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Model::Model(ID3D12Device* device)
    : m_device(device)
{
    // Initialize global transform to identity matrix
    XMStoreFloat4x4(&m_globalTransform, XMMatrixIdentity());
}

Model::~Model()
{
    // Resources are cleaned up automatically via ComPtr
}

bool Model::Initialize()
{
    if (m_vertices.empty() || m_indices.empty())
    {
        return false;
    }

    // Create vertex buffer
    if (!CreateDefaultBuffer(m_vertices, m_vertexBuffer))
    {
        return false;
    }

    // Create index buffer
    if (!CreateDefaultBuffer(m_indices, m_indexBuffer))
    {
        return false;
    }

    // Create vertex buffer view
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(ModelVertex);
    m_vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size() * sizeof(ModelVertex));

    // Create index buffer view
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = static_cast<UINT>(m_indices.size() * sizeof(uint32_t));

    return true;
}

void Model::Bind(ID3D12GraphicsCommandList* commandList)
{
    // Set vertex buffer
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

    // Set index buffer
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // Set primitive topology
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

template<typename T>
bool Model::CreateDefaultBuffer(const std::vector<T>& data, ComPtr<ID3D12Resource>& buffer)
{
    if (data.empty())
    {
        return false;
    }

    UINT bufferSize = static_cast<UINT>(data.size() * sizeof(T));

    // Create upload heap
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    uploadHeapProps.CreationNodeMask = 1;
    uploadHeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> uploadBuffer;
    HRESULT hr = m_device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer));

    if (FAILED(hr))
    {
        return false;
    }

    // Copy data to the upload heap
    void* mappedData = nullptr;
    hr = buffer->Map(0, nullptr, &mappedData);
    if (FAILED(hr))
    {
        return false;
    }

    memcpy(mappedData, data.data(), bufferSize);
    buffer->Unmap(0, nullptr);

    return true;
}