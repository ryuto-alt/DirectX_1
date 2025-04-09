#include "OBJLoader.h"
#include "Material.h"
#include "DXTexture.h"
#include "PathUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace DirectX;

OBJLoader::OBJLoader()
{
}

OBJLoader::~OBJLoader()
{
}

std::unique_ptr<Model> OBJLoader::LoadModel(ID3D12Device* device, const std::string& filename)
{
    // Create a new model
    std::unique_ptr<Model> model = std::make_unique<Model>(device);

    // Temporary storage for OBJ data
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> texCoords;
    std::vector<OBJGroup> groups;
    std::unordered_map<std::string, OBJMaterial> materials;

    // Process the OBJ file
    if (!ProcessOBJFile(filename, positions, normals, texCoords, groups, materials))
    {
        std::cerr << "Failed to process OBJ file: " << filename << std::endl;
        return nullptr;
    }

    // Map to store materials for reuse
    std::unordered_map<std::string, std::shared_ptr<Material>> processedMaterials;

    // Process each group in the OBJ file
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;

    for (const OBJGroup& group : groups)
    {
        // Find or create the material for this group
        std::shared_ptr<Material> material;

        if (processedMaterials.find(group.MaterialName) != processedMaterials.end())
        {
            material = processedMaterials[group.MaterialName];
        }
        else if (materials.find(group.MaterialName) != materials.end())
        {
            const OBJMaterial& objMaterial = materials[group.MaterialName];

            material = std::make_shared<Material>(device);
            material->Initialize();

            // Set material properties
            MaterialConstants& props = material->GetProperties();
            props.Albedo = objMaterial.DiffuseColor;
            props.Metallic = 0.0f; // OBJ doesn't support PBR directly
            props.Roughness = 1.0f - objMaterial.SpecularExponent / 100.0f; // Approximate

            // TODO: Load textures if they exist
            // For simplicity, we're skipping actual texture loading here

            processedMaterials[group.MaterialName] = material;
        }
        else
        {
            // Create a default material if none is specified
            material = std::make_shared<Material>(device);
            material->Initialize();
        }

        // Create submesh for this group
        SubMesh submesh;
        submesh.Material = material;
        submesh.StartIndexLocation = static_cast<UINT>(indices.size());
        submesh.BaseVertexLocation = static_cast<INT>(vertices.size());

        // Store the vertex and index data for this group
        for (const OBJFace& face : group.Faces)
        {
            // Triangulate the face (assuming all faces are triangles or quads)
            for (size_t i = 2; i < face.Indices.size(); ++i)
            {
                // Triangle: 0, i-1, i
                const OBJIndex& idx0 = face.Indices[0];
                const OBJIndex& idx1 = face.Indices[i - 1];
                const OBJIndex& idx2 = face.Indices[i];

                // Add indices
                indices.push_back(static_cast<uint32_t>(vertices.size()));
                indices.push_back(static_cast<uint32_t>(vertices.size() + 1));
                indices.push_back(static_cast<uint32_t>(vertices.size() + 2));

                // Add vertices
                ModelVertex v0, v1, v2;

                // Vertex 0
                v0.Position = (idx0.PositionIndex >= 0 && idx0.PositionIndex < positions.size())
                    ? positions[idx0.PositionIndex]
                    : XMFLOAT3(0, 0, 0);

                v0.Normal = (idx0.NormalIndex >= 0 && idx0.NormalIndex < normals.size())
                    ? normals[idx0.NormalIndex]
                    : XMFLOAT3(0, 1, 0);

                v0.TexCoord = (idx0.TexCoordIndex >= 0 && idx0.TexCoordIndex < texCoords.size())
                    ? texCoords[idx0.TexCoordIndex]
                    : XMFLOAT2(0, 0);

                v0.BoneWeights = XMFLOAT4(1, 0, 0, 0); // No bone weights in OBJ
                v0.BoneIndices = XMUINT4(0, 0, 0, 0);  // No bone indices in OBJ

                // Vertex 1
                v1.Position = (idx1.PositionIndex >= 0 && idx1.PositionIndex < positions.size())
                    ? positions[idx1.PositionIndex]
                    : XMFLOAT3(0, 0, 0);

                v1.Normal = (idx1.NormalIndex >= 0 && idx1.NormalIndex < normals.size())
                    ? normals[idx1.NormalIndex]
                    : XMFLOAT3(0, 1, 0);

                v1.TexCoord = (idx1.TexCoordIndex >= 0 && idx1.TexCoordIndex < texCoords.size())
                    ? texCoords[idx1.TexCoordIndex]
                    : XMFLOAT2(0, 0);

                v1.BoneWeights = XMFLOAT4(1, 0, 0, 0);
                v1.BoneIndices = XMUINT4(0, 0, 0, 0);

                // Vertex 2
                v2.Position = (idx2.PositionIndex >= 0 && idx2.PositionIndex < positions.size())
                    ? positions[idx2.PositionIndex]
                    : XMFLOAT3(0, 0, 0);

                v2.Normal = (idx2.NormalIndex >= 0 && idx2.NormalIndex < normals.size())
                    ? normals[idx2.NormalIndex]
                    : XMFLOAT3(0, 1, 0);

                v2.TexCoord = (idx2.TexCoordIndex >= 0 && idx2.TexCoordIndex < texCoords.size())
                    ? texCoords[idx2.TexCoordIndex]
                    : XMFLOAT2(0, 0);

                v2.BoneWeights = XMFLOAT4(1, 0, 0, 0);
                v2.BoneIndices = XMUINT4(0, 0, 0, 0);

                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v2);
            }
        }

        submesh.IndexCount = static_cast<UINT>(indices.size() - submesh.StartIndexLocation);
        XMStoreFloat4x4(&submesh.LocalTransform, XMMatrixIdentity());

        // Add the submesh to the model
        if (submesh.IndexCount > 0)
        {
            model->GetSubMeshes().push_back(submesh);
        }
    }

    // Set the vertex and index data on the model
    model->GetVertices().assign(vertices.begin(), vertices.end());
    model->GetIndices().assign(indices.begin(), indices.end());

    // Initialize the model
    if (!model->Initialize())
    {
        std::cerr << "Failed to initialize model" << std::endl;
        return nullptr;
    }

    return model;
}

bool OBJLoader::LoadMaterialLibrary(const std::string& mtlFilename, std::unordered_map<std::string, OBJMaterial>& materials)
{
    std::ifstream file(mtlFilename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open material file: " << mtlFilename << std::endl;
        return false;
    }

    OBJMaterial* currentMaterial = nullptr;
    std::string line;

    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "newmtl")
        {
            // Start a new material
            std::string materialName;
            lineStream >> materialName;

            materials[materialName] = OBJMaterial();
            currentMaterial = &materials[materialName];
            currentMaterial->Name = materialName;

            // Set default values
            currentMaterial->AmbientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
            currentMaterial->DiffuseColor = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
            currentMaterial->SpecularColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            currentMaterial->SpecularExponent = 0.0f;
            currentMaterial->Transparency = 1.0f;
        }
        else if (currentMaterial)
        {
            // Process material properties
            if (token == "Ka")
            {
                float r, g, b;
                lineStream >> r >> g >> b;
                currentMaterial->AmbientColor = XMFLOAT4(r, g, b, 1.0f);
            }
            else if (token == "Kd")
            {
                float r, g, b;
                lineStream >> r >> g >> b;
                currentMaterial->DiffuseColor = XMFLOAT4(r, g, b, 1.0f);
            }
            else if (token == "Ks")
            {
                float r, g, b;
                lineStream >> r >> g >> b;
                currentMaterial->SpecularColor = XMFLOAT4(r, g, b, 1.0f);
            }
            else if (token == "Ns")
            {
                lineStream >> currentMaterial->SpecularExponent;
            }
            else if (token == "d" || token == "Tr")
            {
                lineStream >> currentMaterial->Transparency;
                currentMaterial->DiffuseColor.w = currentMaterial->Transparency;
            }
            else if (token == "map_Kd")
            {
                lineStream >> currentMaterial->DiffuseTexture;
            }
            else if (token == "map_Bump" || token == "bump")
            {
                lineStream >> currentMaterial->NormalTexture;
            }
            else if (token == "map_Ks")
            {
                lineStream >> currentMaterial->SpecularTexture;
            }
        }
    }

    return true;
}

bool OBJLoader::ProcessOBJFile(const std::string& filename,
    std::vector<XMFLOAT3>& positions,
    std::vector<XMFLOAT3>& normals,
    std::vector<XMFLOAT2>& texCoords,
    std::vector<OBJGroup>& groups,
    std::unordered_map<std::string, OBJMaterial>& materials)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open OBJ file: " << filename << std::endl;
        return false;
    }

    // ディレクトリパスの取得
    std::string basePath = PathUtils::GetDirectoryPath(filename);

    // Initialize with a default group
    OBJGroup defaultGroup;
    defaultGroup.Name = "default";
    groups.push_back(defaultGroup);

    OBJGroup* currentGroup = &groups.back();
    std::string line;

    // OBJ indices are 1-based, so add a dummy entry at index 0
    positions.push_back(XMFLOAT3(0, 0, 0));
    normals.push_back(XMFLOAT3(0, 0, 0));
    texCoords.push_back(XMFLOAT2(0, 0));

    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream lineStream(line);
        std::string token;
        lineStream >> token;

        if (token == "v")
        {
            // Vertex position
            float x, y, z;
            lineStream >> x >> y >> z;
            positions.push_back(XMFLOAT3(x, y, z));
        }
        else if (token == "vn")
        {
            // Vertex normal
            float x, y, z;
            lineStream >> x >> y >> z;
            normals.push_back(XMFLOAT3(x, y, z));
        }
        else if (token == "vt")
        {
            // Texture coordinate
            float u, v;
            lineStream >> u >> v;
            texCoords.push_back(XMFLOAT2(u, 1.0f - v)); // Flip v for DirectX
        }
        else if (token == "f")
        {
            // Face
            OBJFace face;
            std::string vertexStr;

            while (lineStream >> vertexStr)
            {
                OBJIndex index;
                index.PositionIndex = -1;
                index.TexCoordIndex = -1;
                index.NormalIndex = -1;

                // Parse the vertex index format (pos/tex/norm)
                std::istringstream vertexStream(vertexStr);
                std::string indexStr;

                // Parse position index
                if (std::getline(vertexStream, indexStr, '/'))
                {
                    if (!indexStr.empty())
                    {
                        index.PositionIndex = std::stoi(indexStr);
                    }
                }

                // Parse texture coordinate index
                if (std::getline(vertexStream, indexStr, '/'))
                {
                    if (!indexStr.empty())
                    {
                        index.TexCoordIndex = std::stoi(indexStr);
                    }
                }

                // Parse normal index
                if (std::getline(vertexStream, indexStr, '/'))
                {
                    if (!indexStr.empty())
                    {
                        index.NormalIndex = std::stoi(indexStr);
                    }
                }

                face.Indices.push_back(index);
            }

            if (face.Indices.size() >= 3)
            {
                currentGroup->Faces.push_back(face);
            }
        }
        else if (token == "g" || token == "o")
        {
            // Group or object
            std::string name;
            lineStream >> name;

            OBJGroup group;
            group.Name = name;
            group.MaterialName = currentGroup->MaterialName; // Inherit material

            groups.push_back(group);
            currentGroup = &groups.back();
        }
        else if (token == "mtllib")
        {
            // Material library
            std::string mtlFilename;
            lineStream >> mtlFilename;

            // Resolve the path relative to the OBJ file
            std::string fullPath = PathUtils::CombinePath(basePath, mtlFilename);
            LoadMaterialLibrary(fullPath, materials);
        }
        else if (token == "usemtl")
        {
            // Use material
            std::string materialName;
            lineStream >> materialName;

            currentGroup->MaterialName = materialName;
        }
    }

    return true;
}