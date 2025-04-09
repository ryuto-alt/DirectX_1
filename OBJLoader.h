#pragma once
#include "Model.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// OBJLoader loads 3D models from Wavefront .obj files
class OBJLoader : public ModelLoader {
public:
    OBJLoader();
    ~OBJLoader();

    // Load a model from a .obj file
    std::unique_ptr<Model> LoadModel(ID3D12Device* device, const std::string& filename) override;

private:
    // Helper structs for OBJ parsing
    struct OBJIndex {
        int PositionIndex;
        int TexCoordIndex;
        int NormalIndex;
    };

    struct OBJFace {
        std::vector<OBJIndex> Indices;
    };

    struct OBJMaterial {
        std::string Name;
        DirectX::XMFLOAT4 AmbientColor;
        DirectX::XMFLOAT4 DiffuseColor;
        DirectX::XMFLOAT4 SpecularColor;
        float SpecularExponent;
        float Transparency;
        std::string DiffuseTexture;
        std::string NormalTexture;
        std::string SpecularTexture;
    };

    struct OBJGroup {
        std::string Name;
        std::string MaterialName;
        std::vector<OBJFace> Faces;
    };

    // Helper methods for parsing
    bool LoadMaterialLibrary(const std::string& mtlFilename, std::unordered_map<std::string, OBJMaterial>& materials);
    bool ProcessOBJFile(const std::string& filename,
        std::vector<DirectX::XMFLOAT3>& positions,
        std::vector<DirectX::XMFLOAT3>& normals,
        std::vector<DirectX::XMFLOAT2>& texCoords,
        std::vector<OBJGroup>& groups,
        std::unordered_map<std::string, OBJMaterial>& materials);
};