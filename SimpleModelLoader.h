#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>

// シンプルなモデルローダーの実装
// OBJのみをサポートし、依存関係を最小限に抑えたバージョン

// 簡易的な頂点構造体
struct SimpleVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
};

// シンプルなモデルクラス
class SimpleModel {
public:
    SimpleModel(ID3D12Device* device) : m_device(device) {}
    ~SimpleModel() {}

    // OBJファイルからモデルをロード
    bool LoadFromOBJ(const std::string& filename) {
        std::vector<DirectX::XMFLOAT3> positions;
        std::vector<DirectX::XMFLOAT3> normals;
        std::vector<DirectX::XMFLOAT2> texCoords;
        std::vector<SimpleVertex> vertices;
        std::vector<uint32_t> indices;

        // OBJファイルを開く
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "v") { // 頂点位置
                float x, y, z;
                iss >> x >> y >> z;
                positions.push_back(DirectX::XMFLOAT3(x, y, z));
            }
            else if (token == "vn") { // 法線
                float x, y, z;
                iss >> x >> y >> z;
                normals.push_back(DirectX::XMFLOAT3(x, y, z));
            }
            else if (token == "vt") { // テクスチャ座標
                float u, v;
                iss >> u >> v;
                texCoords.push_back(DirectX::XMFLOAT2(u, 1.0f - v)); // DirectX用にV反転
            }
            else if (token == "f") { // 面
                // OBJのインデックスは1から始まるので注意
                std::string vertexStr;
                std::vector<int> faceIndices;

                while (iss >> vertexStr) {
                    int posIndex = -1, texIndex = -1, normIndex = -1;
                    sscanf_s(vertexStr.c_str(), "%d/%d/%d", &posIndex, &texIndex, &normIndex);

                    // インデックスはOBJでは1始まりなので0始まりに変換
                    posIndex = (posIndex > 0) ? posIndex - 1 : positions.size() + posIndex;
                    texIndex = (texIndex > 0) ? texIndex - 1 : texCoords.size() + texIndex;
                    normIndex = (normIndex > 0) ? normIndex - 1 : normals.size() + normIndex;

                    // 頂点を作成
                    SimpleVertex vertex;

                    // 頂点位置
                    if (posIndex >= 0 && posIndex < positions.size()) {
                        vertex.Position = positions[posIndex];
                    }

                    // 法線
                    if (normIndex >= 0 && normIndex < normals.size()) {
                        vertex.Normal = normals[normIndex];
                    }
                    else {
                        vertex.Normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); // デフォルト上向き
                    }

                    // テクスチャ座標
                    if (texIndex >= 0 && texIndex < texCoords.size()) {
                        vertex.TexCoord = texCoords[texIndex];
                    }
                    else {
                        vertex.TexCoord = DirectX::XMFLOAT2(0.0f, 0.0f); // デフォルト左上
                    }

                    // 頂点を追加
                    vertices.push_back(vertex);
                    faceIndices.push_back(static_cast<int>(vertices.size() - 1));
                }

                // 三角形分割（三角形ファンとして処理）
                for (size_t i = 2; i < faceIndices.size(); ++i) {
                    indices.push_back(faceIndices[0]);
                    indices.push_back(faceIndices[i - 1]);
                    indices.push_back(faceIndices[i]);
                }
            }
        }

        file.close();

        // モデルデータをGPUバッファに転送
        return CreateBuffers(vertices, indices);
    }

    // モデルを描画
    void Draw(ID3D12GraphicsCommandList* commandList) {
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        commandList->IASetIndexBuffer(&m_indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
    }

private:
    // バッファを作成
    bool CreateBuffers(const std::vector<SimpleVertex>& vertices, const std::vector<uint32_t>& indices) {
        if (vertices.empty() || indices.empty()) {
            return false;
        }

        m_indexCount = static_cast<UINT>(indices.size());

        // ヒーププロパティ設定
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 頂点バッファの作成
        D3D12_RESOURCE_DESC vbDesc = {};
        vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vbDesc.Width = vertices.size() * sizeof(SimpleVertex);
        vbDesc.Height = 1;
        vbDesc.DepthOrArraySize = 1;
        vbDesc.MipLevels = 1;
        vbDesc.Format = DXGI_FORMAT_UNKNOWN;
        vbDesc.SampleDesc.Count = 1;
        vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (FAILED(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &vbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)))) {
            return false;
        }

        // 頂点データをマップしてコピー
        void* mappedVertexData = nullptr;
        if (FAILED(m_vertexBuffer->Map(0, nullptr, &mappedVertexData))) {
            return false;
        }
        memcpy(mappedVertexData, vertices.data(), vertices.size() * sizeof(SimpleVertex));
        m_vertexBuffer->Unmap(0, nullptr);

        // 頂点バッファビューの設定
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(SimpleVertex);
        m_vertexBufferView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(SimpleVertex));

        // インデックスバッファの作成
        D3D12_RESOURCE_DESC ibDesc = vbDesc;
        ibDesc.Width = indices.size() * sizeof(uint32_t);

        if (FAILED(m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &ibDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)))) {
            return false;
        }

        // インデックスデータをマップしてコピー
        void* mappedIndexData = nullptr;
        if (FAILED(m_indexBuffer->Map(0, nullptr, &mappedIndexData))) {
            return false;
        }
        memcpy(mappedIndexData, indices.data(), indices.size() * sizeof(uint32_t));
        m_indexBuffer->Unmap(0, nullptr);

        // インデックスバッファビューの設定
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_indexBufferView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(uint32_t));

        return true;
    }

    ID3D12Device* m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
    UINT m_indexCount = 0;
};