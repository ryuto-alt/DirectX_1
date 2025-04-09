#pragma once
#include "Model.h"
#include <string>
#include <memory>
#include <fbxsdk.h>

// Forward declarations
class Skeleton;
class Animation;

// FBXLoader loads 3D models from Autodesk FBX files
class FBXLoader : public ModelLoader {
public:
    FBXLoader();
    ~FBXLoader();

    // Load a model from an FBX file
    std::unique_ptr<Model> LoadModel(ID3D12Device* device, const std::string& filename) override;

private:
    // Initialize the FBX SDK
    bool Initialize();

    // Process an FBX scene
    bool ProcessScene(ID3D12Device* device, FbxScene* scene, Model* model);

    // Process an FBX mesh
    bool ProcessMesh(ID3D12Device* device, FbxNode* node, Model* model);

    // Process the skeleton from an FBX mesh
    std::shared_ptr<Skeleton> ProcessSkeleton(FbxMesh* mesh);

    // Process animations from an FBX scene
    void ProcessAnimations(FbxScene* scene, Model* model);

    // Convert FBX matrix to DirectX matrix
    DirectX::XMFLOAT4X4 ConvertFbxMatrix(const FbxAMatrix& matrix);

    // Helper to create a material from FBX properties
    std::shared_ptr<Material> CreateMaterialFromFbxProperties(ID3D12Device* device, FbxSurfaceMaterial* fbxMaterial);

    // Helper to load texture from file
    std::shared_ptr<DXTexture> LoadTexture(ID3D12Device* device, const std::string& filename);

    // FBX SDK objects
    FbxManager* m_fbxManager;
    FbxIOSettings* m_fbxIOSettings;
    FbxImporter* m_fbxImporter;
};