#include "ResourceManager.h"
#include "Engine.h"
#include "Renderer.h"

#include <d3dx12.h>
#include <cassert>
#include <fstream>
#include <system_error>
#include <filesystem>
#include <DirectXTex.h>

// OBJモデル読み込み用ヘルパー
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <d3dcompiler.h>

// MP3/OGG読み込み用ヘルパー
// Note: 実際の実装ではライブラリ（libvorbis, minimp3など）をインクルードする必要があります

ResourceManager::ResourceManager()
    : m_device(nullptr)
    , m_commandList(nullptr)
    , m_currentDescriptorIndex(0)
    , m_initialized(false)
{
}

ResourceManager::~ResourceManager()
{
    Shutdown();
}

bool ResourceManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    assert(device != nullptr);
    assert(commandList != nullptr);

    m_device = device;
    m_commandList = commandList;
    m_currentDescriptorIndex = 0;
    m_initialized = true;

    return true;
}

void ResourceManager::Shutdown()
{
    // リソースキャッシュをクリア
    m_textures.clear();
    m_models.clear();
    m_audioClips.clear();
    m_shaders.clear();

    m_initialized = false;
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::wstring& filename)
{
    // 既に読み込まれているか確認
    auto it = m_textures.find(filename);
    if (it != m_textures.end()) {
        return it->second;
    }

    // 新しいテクスチャを作成
    std::shared_ptr<Texture> texture = std::make_shared<Texture>();
    texture->name = filename;

    // DirectXTexを使用してテクスチャを読み込む
    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratchImage = {};

    // ファイル拡張子を取得
    std::wstring extension = std::filesystem::path(filename).extension().wstring();
    HRESULT hr = S_OK;

    if (extension == L".dds") {
        hr = DirectX::LoadFromDDSFile(filename.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
    }
    else if (extension == L".tga") {
        hr = DirectX::LoadFromTGAFile(filename.c_str(), &metadata, scratchImage);
    }
    else if (extension == L".hdr") {
        hr = DirectX::LoadFromHDRFile(filename.c_str(), &metadata, scratchImage);
    }
    else {
        // その他のフォーマット（png, jpg, bmp, gif, tiff, etc）
        hr = DirectX::LoadFromWICFile(filename.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);
    }

    if (FAILED(hr)) {
        // エラー処理
        return nullptr;
    }

    // メタデータをコピー
    texture->metadata = metadata;

    // テクスチャリソースを作成
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    textureDesc.Format = metadata.format;
    textureDesc.Width = metadata.width;
    textureDesc.Height = static_cast<UINT>(metadata.height);
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture->resource)
    );

    if (FAILED(hr)) {
        // エラー処理
        return nullptr;
    }

    // テクスチャデータのアップロード
    const DirectX::Image* image = scratchImage.GetImages();
    UINT64 totalBytes = 0;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(metadata.mipLevels);
    std::vector<UINT> numRows(metadata.mipLevels);
    std::vector<UINT64> rowSizeInBytes(metadata.mipLevels);

    m_device->GetCopyableFootprints(
        &textureDesc,
        0,
        metadata.mipLevels,
        0,
        layouts.data(),
        numRows.data(),
        rowSizeInBytes.data(),
        &totalBytes
    );

    // アップロードヒープを作成
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

    hr = m_device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&texture->uploadHeap)
    );

    if (FAILED(hr)) {
        // エラー処理
        return nullptr;
    }

    // アップロードヒープにデータをマップ
    BYTE* mappedData = nullptr;
    CD3DX12_RANGE readRange(0, 0);

    hr = texture->uploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
    if (FAILED(hr)) {
        // エラー処理
        return nullptr;
    }

    // 各サブリソースのデータをコピー
    for (UINT i = 0; i < metadata.mipLevels; ++i) {
        const DirectX::Image& subImage = image[i];
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layouts[i];
        UINT numRows = static_cast<UINT>(subImage.height);

        // データをコピー
        for (UINT row = 0; row < numRows; ++row) {
            memcpy(
                mappedData + layout.Offset + row * layout.Footprint.RowPitch,
                subImage.pixels + row * subImage.rowPitch,
                subImage.rowPitch
            );
        }
    }

    texture->uploadHeap->Unmap(0, nullptr);

    // コマンドリストにコピーコマンドを追加
    for (UINT i = 0; i < metadata.mipLevels; ++i) {
        CD3DX12_TEXTURE_COPY_LOCATION dst(texture->resource.Get(), i);
        CD3DX12_TEXTURE_COPY_LOCATION src(texture->uploadHeap.Get(), layouts[i]);

        m_commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }

    // リソースバリアを設定して、テクスチャをシェーダリソースとして使用できるようにする
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture->resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    m_commandList->ResourceBarrier(1, &barrier);

    // CBV/SRV/UAVヒープからディスクリプタを割り当て
    texture->descriptorIndex = m_currentDescriptorIndex++;

    // SRVを作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;

    switch (metadata.dimension) {
    case DirectX::TEX_DIMENSION_TEXTURE1D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        srvDesc.Texture1D.MipLevels = textureDesc.MipLevels;
        break;

    case DirectX::TEX_DIMENSION_TEXTURE2D:
        if (metadata.arraySize > 1) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MipLevels = textureDesc.MipLevels;
            srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(metadata.arraySize);
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
        }
        break;

    case DirectX::TEX_DIMENSION_TEXTURE3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = textureDesc.MipLevels;
        break;

    default:
        // エラー処理
        return nullptr;
    }

    // SRVを作成
    ID3D12DescriptorHeap* cbvSrvUavHeap = GetCbvSrvUavHeap();
    if (cbvSrvUavHeap) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
            cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(),
            texture->descriptorIndex,
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        );

        m_device->CreateShaderResourceView(texture->resource.Get(), &srvDesc, handle);
    }

    // キャッシュに追加
    m_textures[filename] = texture;

    return texture;
}

std::shared_ptr<Model> ResourceManager::LoadModel(const std::wstring& filename)
{
    // 拡張子でファイル形式を判断
    std::wstring extension = std::filesystem::path(filename).extension();

    if (extension == L".obj") {
        return LoadObjModel(filename);
    }
    else if (extension == L".fbx") {
        return LoadFbxModel(filename);
    }
    else {
        // サポートされていない形式
        return nullptr;
    }
}

std::shared_ptr<Model> ResourceManager::LoadObjModel(const std::wstring& filename)
{
    // 既に読み込まれているか確認
    auto it = m_models.find(filename);
    if (it != m_models.end()) {
        return it->second;
    }

    // 新しいモデルを作成
    std::shared_ptr<Model> model = std::make_shared<Model>();
    model->name = filename;
    model->position = DirectX::XMFLOAT3(0, 0, 0);
    model->rotation = DirectX::XMFLOAT3(0, 0, 0);
    model->scale = DirectX::XMFLOAT3(1, 1, 1);

    // ファイルパスを取得
    std::string filenameU8(filename.begin(), filename.end());
    std::string directory = std::filesystem::path(filenameU8).parent_path().string() + "/";

    // tinyobjloaderを使用してOBJファイルを読み込む
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    std::string warn;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filenameU8.c_str(), directory.c_str());

    if (!err.empty()) {
        // エラー処理
        return nullptr;
    }

    if (!ret) {
        // 読み込み失敗
        return nullptr;
    }

    // 材質の読み込み
    std::vector<std::shared_ptr<Material>> modelMaterials;
    modelMaterials.reserve(materials.size());

    for (const auto& material : materials) {
        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat->name = std::wstring(material.name.begin(), material.name.end());
        mat->diffuse = DirectX::XMFLOAT4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f);
        mat->ambient = DirectX::XMFLOAT4(material.ambient[0], material.ambient[1], material.ambient[2], 1.0f);
        mat->specular = DirectX::XMFLOAT4(material.specular[0], material.specular[1], material.specular[2], 1.0f);
        mat->shininess = material.shininess;

        // テクスチャの読み込み
        if (!material.diffuse_texname.empty()) {
            std::string diffuseTexPath = directory + material.diffuse_texname;
            std::wstring wTexPath(diffuseTexPath.begin(), diffuseTexPath.end());
            mat->diffuseMap = LoadTexture(wTexPath);
        }

        if (!material.normal_texname.empty()) {
            std::string normalTexPath = directory + material.normal_texname;
            std::wstring wTexPath(normalTexPath.begin(), normalTexPath.end());
            mat->normalMap = LoadTexture(wTexPath);
        }

        if (!material.specular_texname.empty()) {
            std::string specularTexPath = directory + material.specular_texname;
            std::wstring wTexPath(specularTexPath.begin(), specularTexPath.end());
            mat->specularMap = LoadTexture(wTexPath);
        }

        modelMaterials.push_back(mat);
    }

    // デフォルトマテリアル
    if (modelMaterials.empty()) {
        std::shared_ptr<Material> defaultMat = std::make_shared<Material>();
        defaultMat->name = L"default";
        defaultMat->diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
        defaultMat->ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
        defaultMat->specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        defaultMat->shininess = 32.0f;
        modelMaterials.push_back(defaultMat);
    }

    // 各シェイプをメッシュとして処理
    for (const auto& shape : shapes) {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        mesh->name = std::wstring(shape.name.begin(), shape.name.end());

        // 頂点とインデックスのデータを収集
        std::vector<Vertex> vertices;
        std::vector<UINT> indices;

        // インデックスごとに一意の頂点を生成
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            // 位置
            if (index.vertex_index >= 0) {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
            }

            // 法線
            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            // テクスチャ座標
            if (index.texcoord_index >= 0) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Vを反転（DirectX座標系）
                };
            }

            // 頂点を追加
            vertices.push_back(vertex);
            indices.push_back(static_cast<UINT>(indices.size()));
        }

        // 接線と従法線を計算（法線マップ用）
        for (size_t i = 0; i < indices.size(); i += 3) {
            Vertex& v0 = vertices[indices[i]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];

            // エッジベクトル
            DirectX::XMFLOAT3 edge1 = {
                v1.position.x - v0.position.x,
                v1.position.y - v0.position.y,
                v1.position.z - v0.position.z
            };

            DirectX::XMFLOAT3 edge2 = {
                v2.position.x - v0.position.x,
                v2.position.y - v0.position.y,
                v2.position.z - v0.position.z
            };

            // テクスチャ座標の差分
            DirectX::XMFLOAT2 deltaUV1 = {
                v1.texCoord.x - v0.texCoord.x,
                v1.texCoord.y - v0.texCoord.y
            };

            DirectX::XMFLOAT2 deltaUV2 = {
                v2.texCoord.x - v0.texCoord.x,
                v2.texCoord.y - v0.texCoord.y
            };

            // 接線と従法線の計算
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            DirectX::XMFLOAT3 tangent = {
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
            };

            DirectX::XMFLOAT3 bitangent = {
                f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
                f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
                f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
            };

            // 接線と従法線を正規化
            float tangentLength = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
            float bitangentLength = sqrtf(bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z);

            if (tangentLength > 0.0f) {
                tangent.x /= tangentLength;
                tangent.y /= tangentLength;
                tangent.z /= tangentLength;
            }

            if (bitangentLength > 0.0f) {
                bitangent.x /= bitangentLength;
                bitangent.y /= bitangentLength;
                bitangent.z /= bitangentLength;
            }

            // 頂点に接線と従法線を設定
            v0.tangent = tangent;
            v1.tangent = tangent;
            v2.tangent = tangent;

            v0.bitangent = bitangent;
            v1.bitangent = bitangent;
            v2.bitangent = bitangent;
        }

        // マテリアルの割り当て
        int materialId = shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[0];
        if (materialId >= 0 && materialId < static_cast<int>(modelMaterials.size())) {
            mesh->material = modelMaterials[materialId];
        }
        else {
            mesh->material = modelMaterials[0]; // デフォルトマテリアル
        }

        // 頂点バッファの作成
        mesh->vertices = vertices;
        mesh->indices = indices;
        mesh->indexCount = static_cast<UINT>(indices.size());

        // 頂点バッファとインデックスバッファを作成
        const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
        const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(UINT));

        // 頂点バッファのリソースを作成
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexUploadBuffer;
        mesh->vertexBuffer = CreateDefaultBuffer(vertices.data(), vbByteSize, vertexUploadBuffer);

        // 頂点バッファビューを設定
        mesh->vertexBufferView.BufferLocation = mesh->vertexBuffer->GetGPUVirtualAddress();
        mesh->vertexBufferView.StrideInBytes = sizeof(Vertex);
        mesh->vertexBufferView.SizeInBytes = vbByteSize;

        // インデックスバッファのリソースを作成
        Microsoft::WRL::ComPtr<ID3D12Resource> indexUploadBuffer;
        mesh->indexBuffer = CreateDefaultBuffer(indices.data(), ibByteSize, indexUploadBuffer);

        // インデックスバッファビューを設定
        mesh->indexBufferView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
        mesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        mesh->indexBufferView.SizeInBytes = ibByteSize;

        // モデルにメッシュを追加
        model->meshes.push_back(mesh);
    }

    // キャッシュに追加
    m_models[filename] = model;

    return model;
}

std::shared_ptr<Model> ResourceManager::LoadFbxModel(const std::wstring& filename)
{
    // 既に読み込まれているか確認
    auto it = m_models.find(filename);
    if (it != m_models.end()) {
        return it->second;
    }

    // 新しいモデルを作成
    std::shared_ptr<Model> model = std::make_shared<Model>();
    model->name = filename;
    model->position = DirectX::XMFLOAT3(0, 0, 0);
    model->rotation = DirectX::XMFLOAT3(0, 0, 0);
    model->scale = DirectX::XMFLOAT3(1, 1, 1);

    // FBXファイルの読み込み
    // 注：FBXファイルの読み込みには、FBX SDKなどの外部ライブラリが必要です
    // ここでは簡単なスタブ実装のみ提供します

    // デフォルトメッシュとマテリアルを作成（サンプル用）
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
    mesh->name = L"default_fbx_mesh";

    std::shared_ptr<Material> material = std::make_shared<Material>();
    material->name = L"default_fbx_material";
    material->diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    material->ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    material->specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material->shininess = 32.0f;

    mesh->material = material;

    // 立方体のサンプル頂点を作成（FBX読み込みのプレースホルダ）
    std::vector<Vertex> vertices = {
        // 前面
        { {-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
        { {-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },

        // 背面
        { {-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
        { {-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },

        // 上面
        { {-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { {-1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f,  1.0f,  1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f,  1.0f, -1.0f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },

        // 下面
        { {-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { { 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
        { {-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },

        // 右面
        { { 1.0f, -1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f,  1.0f, -1.0f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f,  1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
        { { 1.0f, -1.0f,  1.0f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },

        // 左面
        { {-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
        { {-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
        { {-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
        { {-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} }
    };

    // 立方体のインデックス
    std::vector<UINT> indices = {
        // 前面
        0, 1, 2, 0, 2, 3,
        // 背面
        4, 5, 6, 4, 6, 7,
        // 上面
        8, 9, 10, 8, 10, 11,
        // 下面
        12, 13, 14, 12, 14, 15,
        // 右面
        16, 17, 18, 16, 18, 19,
        // 左面
        20, 21, 22, 20, 22, 23
    };

    // メッシュのデータを設定
    mesh->vertices = vertices;
    mesh->indices = indices;
    mesh->indexCount = static_cast<UINT>(indices.size());

    // 頂点バッファとインデックスバッファを作成
    const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(UINT));

    // 頂点バッファのリソースを作成
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexUploadBuffer;
    mesh->vertexBuffer = CreateDefaultBuffer(vertices.data(), vbByteSize, vertexUploadBuffer);

    // 頂点バッファビューを設定
    mesh->vertexBufferView.BufferLocation = mesh->vertexBuffer->GetGPUVirtualAddress();
    mesh->vertexBufferView.StrideInBytes = sizeof(Vertex);
    mesh->vertexBufferView.SizeInBytes = vbByteSize;

    // インデックスバッファのリソースを作成
    Microsoft::WRL::ComPtr<ID3D12Resource> indexUploadBuffer;
    mesh->indexBuffer = CreateDefaultBuffer(indices.data(), ibByteSize, indexUploadBuffer);

    // インデックスバッファビューを設定
    mesh->indexBufferView.BufferLocation = mesh->indexBuffer->GetGPUVirtualAddress();
    mesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mesh->indexBufferView.SizeInBytes = ibByteSize;

    // モデルにメッシュを追加
    model->meshes.push_back(mesh);

    // キャッシュに追加
    m_models[filename] = model;

    return model;
}

std::shared_ptr<AudioClip> ResourceManager::LoadAudio(const std::wstring& filename)
{
    // 既に読み込まれているか確認
    auto it = m_audioClips.find(filename);
    if (it != m_audioClips.end()) {
        return it->second;
    }

    // 新しいオーディオクリップを作成
    std::shared_ptr<AudioClip> audioClip = std::make_shared<AudioClip>();
    audioClip->name = filename;

    // 拡張子で形式を判別
    std::wstring extension = std::filesystem::path(filename).extension();
    bool success = false;

    if (extension == L".wav") {
        success = LoadWavFile(filename, audioClip);
    }
    else if (extension == L".mp3") {
        success = LoadMp3File(filename, audioClip);
    }
    else if (extension == L".ogg") {
        success = LoadOggFile(filename, audioClip);
    }
    else {
        // サポートされていない形式
        return nullptr;
    }

    if (!success) {
        return nullptr;
    }

    // キャッシュに追加
    m_audioClips[filename] = audioClip;

    return audioClip;
}

bool ResourceManager::LoadWavFile(const std::wstring& filename, std::shared_ptr<AudioClip> audioClip)
{
    // WAVファイルの読み込み
    // 注: 完全な実装は別途WASAPIやXAudio2などのライブラリが必要

    struct WavHeader {
        char riffId[4];          // "RIFF"
        UINT fileSize;           // ファイルサイズ - 8
        char waveId[4];          // "WAVE"
        char fmtId[4];           // "fmt "
        UINT fmtSize;            // fmtチャンクのサイズ
        UINT16 format;           // フォーマットID (1 = PCM)
        UINT16 channels;         // チャンネル数
        UINT sampleRate;         // サンプルレート
        UINT byteRate;           // バイトレート (サンプルレート * ブロックサイズ)
        UINT16 blockAlign;       // ブロックサイズ (ビット深度 * チャンネル / 8)
        UINT16 bitsPerSample;    // ビット深度
        // dataチャンクが続く
    };

    // ファイルを開く
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // ヘッダーを読み込む
    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    // "RIFF"と"WAVE"を確認
    if (memcmp(header.riffId, "RIFF", 4) != 0 || memcmp(header.waveId, "WAVE", 4) != 0) {
        return false;
    }

    // "data"チャンクを見つける
    char chunkId[4];
    UINT chunkSize;

    while (file.read(chunkId, 4) && file.read(reinterpret_cast<char*>(&chunkSize), 4)) {
        if (memcmp(chunkId, "data", 4) == 0) {
            break;
        }

        // このチャンクをスキップ
        file.seekg(chunkSize, std::ios::cur);
    }

    if (memcmp(chunkId, "data", 4) != 0) {
        return false;
    }

    // オーディオデータを読み込む
    audioClip->data.resize(chunkSize);
    file.read(reinterpret_cast<char*>(audioClip->data.data()), chunkSize);

    audioClip->channelCount = header.channels;
    audioClip->sampleRate = header.sampleRate;
    audioClip->bitsPerSample = header.bitsPerSample;

    return true;
}

bool ResourceManager::LoadMp3File(const std::wstring& filename, std::shared_ptr<AudioClip> audioClip)
{
    // MP3ファイルの読み込み
    // 注：実際の実装では、MP3デコーダーライブラリ（例：minimp3）を使用する必要があります

    // 簡易的な実装（実際のMP3デコードは行わない）
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 全データを読み込む（デコードは行わない）
    audioClip->data.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(audioClip->data.data()), size)) {
        return false;
    }

    // デフォルト値（実際はMP3ヘッダーから取得する必要がある）
    audioClip->channelCount = 2;        // ステレオと仮定
    audioClip->sampleRate = 44100;      // 44.1kHzと仮定
    audioClip->bitsPerSample = 16;      // 16ビットと仮定

    return true;
}

bool ResourceManager::LoadOggFile(const std::wstring& filename, std::shared_ptr<AudioClip> audioClip)
{
    // OGGファイルの読み込み
    // 注：実際の実装では、OGG/Vorbisデコーダーライブラリ（例：libvorbis）を使用する必要があります

    // 簡易的な実装（実際のOGGデコードは行わない）
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 全データを読み込む（デコードは行わない）
    audioClip->data.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(audioClip->data.data()), size)) {
        return false;
    }

    // デフォルト値（実際はOGGヘッダーから取得する必要がある）
    audioClip->channelCount = 2;        // ステレオと仮定
    audioClip->sampleRate = 44100;      // 44.1kHzと仮定
    audioClip->bitsPerSample = 16;      // 16ビットと仮定

    return true;
}

std::shared_ptr<Shader> ResourceManager::LoadShader(
    const std::wstring& name,
    const std::wstring& vsFilename,
    const std::wstring& psFilename,
    const std::wstring& gsFilename,
    const std::wstring& hsFilename,
    const std::wstring& dsFilename,
    const std::wstring& csFilename)
{
    // 既に読み込まれているか確認
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }

    std::shared_ptr<Shader> shader = std::make_shared<Shader>();
    shader->name = name;

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 頂点シェーダーの読み込み
    if (!vsFilename.empty()) {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            vsFilename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "vs_5_1",
            compileFlags,
            0,
            &shader->vertexShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return nullptr;
        }
    }

    // ピクセルシェーダーの読み込み
    if (!psFilename.empty()) {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            psFilename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "ps_5_1",
            compileFlags,
            0,
            &shader->pixelShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return nullptr;
        }
    }

    // ジオメトリシェーダーの読み込み（オプション）
    if (!gsFilename.empty()) {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            gsFilename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "gs_5_1",
            compileFlags,
            0,
            &shader->geometryShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            // ジオメトリシェーダーは必須ではないので続行
        }
    }

    // ハルシェーダーの読み込み（オプション）
    if (!hsFilename.empty()) {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            hsFilename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "hs_5_1",
            compileFlags,
            0,
            &shader->hullShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            // ハルシェーダーは必須ではないので続行
        }
    }

    // ドメインシェーダーの読み込み（オプション）
    if (!dsFilename.empty()) {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            dsFilename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "ds_5_1",
            compileFlags,
            0,
            &shader->domainShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            // ドメインシェーダーは必須ではないので続行
        }
    }

    // コンピュートシェーダーの読み込み（オプション）
    if (!csFilename.empty()) {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            csFilename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "cs_5_1",
            compileFlags,
            0,
            &shader->computeShaderBlob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            // コンピュートシェーダーは必須ではないので続行
        }
    }

    // キャッシュに追加
    m_shaders[name] = shader;

    return shader;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateUploadBuffer(const void* data, UINT64 dataSize)
{
    assert(m_device != nullptr);

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

    // アップロードヒープの作成
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );

    if (FAILED(hr)) {
        return nullptr;
    }

    // データをアップロードバッファにコピー
    if (data != nullptr) {
        void* mappedData = nullptr;
        CD3DX12_RANGE readRange(0, 0); // マップの目的はデータの書き込みのみ

        hr = uploadBuffer->Map(0, &readRange, &mappedData);
        if (FAILED(hr)) {
            return nullptr;
        }

        memcpy(mappedData, data, dataSize);
        uploadBuffer->Unmap(0, nullptr);
    }

    return uploadBuffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateDefaultBuffer(
    const void* data,
    UINT64 dataSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    assert(m_device != nullptr);
    assert(m_commandList != nullptr);

    Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

    // デフォルトヒープの作成
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

    HRESULT hr = m_device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&defaultBuffer)
    );

    if (FAILED(hr)) {
        return nullptr;
    }

    // アップロードヒープの作成
    uploadBuffer = CreateUploadBuffer(data, dataSize);
    if (!uploadBuffer) {
        return nullptr;
    }

    // リソースバリアを設定して、コピー先としてデフォルトバッファを準備
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    m_commandList->ResourceBarrier(1, &barrier);

    // アップロードバッファからデフォルトバッファにデータをコピー
    m_commandList->CopyResource(defaultBuffer.Get(), uploadBuffer.Get());

    // リソースバリアを設定して、一般的なアクセスのためにデフォルトバッファを準備
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    m_commandList->ResourceBarrier(1, &barrier);

    return defaultBuffer;
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::wstring& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::wstring& name)
{
    auto it = m_models.find(name);
    if (it != m_models.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<AudioClip> ResourceManager::GetAudio(const std::wstring& name)
{
    auto it = m_audioClips.find(name);
    if (it != m_audioClips.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::wstring& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }
    return nullptr;
}

ID3D12DescriptorHeap* ResourceManager::GetCbvSrvUavHeap() const
{
    // Engine経由でRendererからヒープを取得
    Renderer* renderer = Engine::GetInstance().GetRenderer();
    if (renderer) {
        return renderer->GetCbvSrvUavHeap();
    }
    return nullptr;
}

void ResourceManager::ExecuteUploadHeap()
{
    assert(m_commandList != nullptr);

    // コマンドリストを閉じて実行
    m_commandList->Close();

    ID3D12CommandList* commandLists[] = { m_commandList };
    ID3D12CommandQueue* commandQueue = Engine::GetInstance().GetCommandQueue();
    if (commandQueue) {
        commandQueue->ExecuteCommandLists(1, commandLists);

        // GPUの処理完了を待機
        Engine::GetInstance().GetRenderer()->WaitForGpu();
    }

    // コマンドリストをリセット
    UINT currentBackBuffer = Engine::GetInstance().GetRenderer()->GetBackBufferCount() - 1;
    m_commandList->Reset(Engine::GetInstance().GetRenderer()->GetCommandList()->GetCommandAllocator(), nullptr);
}