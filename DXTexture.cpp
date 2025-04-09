#include "DXTexture.h"
#include <d3d12.h>
#include <DirectXTex.h>
#include <wrl/client.h>
#include <string>
#include <iostream>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

DXTexture::DXTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
    : m_device(device), m_commandList(commandList)
{
}

DXTexture::~DXTexture()
{
    // ComPtr で自動解放
}

bool DXTexture::LoadFromFile(const std::wstring& filename)
{
    // DirectXTex の LoadFromWICFile で画像ファイル読み込み
    HRESULT hr = LoadFromWICFile(
        filename.c_str(),
        WIC_FLAGS_NONE,
        &m_metadata,
        m_scratchImage);

    if (FAILED(hr)) {
        std::wcerr << L"LoadFromWICFile failed for " << filename
            << L", hr = 0x" << std::hex << hr << std::endl;
        return false;
    }

    // 画像の1枚目を取得
    const Image* image = m_scratchImage.GetImage(0, 0, 0);
    if (!image) {
        std::cerr << "Failed to get image from scratchImage" << std::endl;
        return false;
    }

    // テクスチャ用リソース作成
    if (!CreateTextureResource(m_metadata)) {
        std::cerr << "CreateTextureResource failed" << std::endl;
        return false;
    }

    // アップロードヒープ作成
    if (!CreateTextureUploadHeap(image)) {
        std::cerr << "CreateTextureUploadHeap failed" << std::endl;
        return false;
    }

    // アップロードヒープからテクスチャへコピー
    if (!CopyTextureData(image)) {
        std::cerr << "CopyTextureData failed" << std::endl;
        return false;
    }

    return true;
}

D3D12_SHADER_RESOURCE_VIEW_DESC DXTexture::GetSrvDesc() const
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    return srvDesc;
}

bool DXTexture::CreateTextureResource(const TexMetadata& metadata)
{
    // テクスチャリソースは GPU バインド用なので DEFAULT ヒープを使う
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // リソースディスクリプタの設定
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    resourceDesc.Alignment = 0;
    resourceDesc.Width = static_cast<UINT>(metadata.width);
    resourceDesc.Height = static_cast<UINT>(metadata.height);
    resourceDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    resourceDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    resourceDesc.Format = metadata.format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_textureResource));

    if (FAILED(hr)) {
        std::cerr << "CreateCommittedResource for texture failed, hr = 0x"
            << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool DXTexture::CreateTextureUploadHeap(const Image* image)
{
    // アップロード用ヒープは UPLOAD ヒープタイプを使う
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = AlignmentedSize(image->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * image->height;
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
        IID_PPV_ARGS(&m_textureUploadHeap));

    if (FAILED(hr)) {
        std::cerr << "CreateCommittedResource for upload heap failed, hr = 0x"
            << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool DXTexture::CopyTextureData(const Image* image)
{
    UINT8* pUploadHeapData = nullptr;
    HRESULT hr = m_textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pUploadHeapData));
    if (FAILED(hr)) {
        std::cerr << "Mapping upload heap failed, hr = 0x"
            << std::hex << hr << std::endl;
        return false;
    }

    UINT64 alignedRowPitch = AlignmentedSize(image->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

    UINT8* pSrcData = image->pixels;
    for (size_t h = 0; h < image->height; ++h)
    {
        memcpy(pUploadHeapData + alignedRowPitch * h, pSrcData, image->rowPitch);
        pSrcData += image->rowPitch;
    }

    m_textureUploadHeap->Unmap(0, nullptr);

    // コピー元の設定
    D3D12_TEXTURE_COPY_LOCATION copySource = {};
    copySource.pResource = m_textureUploadHeap.Get();
    copySource.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    copySource.PlacedFootprint.Offset = 0;
    copySource.PlacedFootprint.Footprint.Width = static_cast<UINT>(image->width);
    copySource.PlacedFootprint.Footprint.Height = static_cast<UINT>(image->height);
    copySource.PlacedFootprint.Footprint.Depth = 1;
    copySource.PlacedFootprint.Footprint.RowPitch = static_cast<UINT>(alignedRowPitch);
    copySource.PlacedFootprint.Footprint.Format = image->format;

    // コピー先の設定
    D3D12_TEXTURE_COPY_LOCATION copyDest = {};
    copyDest.pResource = m_textureResource.Get();
    copyDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    copyDest.SubresourceIndex = 0;

    m_commandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySource, nullptr);

    // コピー完了後、テクスチャリソースをピクセルシェーダーで利用できる状態にする
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_textureResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrier);

    return true;
}

size_t DXTexture::AlignmentedSize(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}
