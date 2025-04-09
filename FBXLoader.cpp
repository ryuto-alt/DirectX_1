#define NOMINMAX
#include "FBXLoader.h"
#include "Material.h"
#include "DXTexture.h"
#include "Skeleton.h"
#include "Animation.h"
#include "PathUtils.h"
#include <iostream>

using namespace DirectX;

FBXLoader::FBXLoader()
    : m_fbxManager(nullptr), m_fbxIOSettings(nullptr), m_fbxImporter(nullptr)
{
    Initialize();
}

FBXLoader::~FBXLoader()
{
    // Clean up FBX SDK objects
    if (m_fbxImporter)
    {
        m_fbxImporter->Destroy();
        m_fbxImporter = nullptr;
    }

    if (m_fbxIOSettings)
    {
        m_fbxIOSettings->Destroy();
        m_fbxIOSettings = nullptr;
    }

    if (m_fbxManager)
    {
        m_fbxManager->Destroy();
        m_fbxManager = nullptr;
    }
}

bool FBXLoader::Initialize()
{
    // Initialize the FBX SDK
    m_fbxManager = FbxManager::Create();
    if (!m_fbxManager)
    {
        std::cerr << "Failed to create FBX Manager" << std::endl;
        return false;
    }

    // Create the IO settings object
    m_fbxIOSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
    m_fbxManager->SetIOSettings(m_fbxIOSettings);

    // Create an importer using the SDK manager
    m_fbxImporter = FbxImporter::Create(m_fbxManager, "");
    if (!m_fbxImporter)
    {
        std::cerr << "Failed to create FBX Importer" << std::endl;
        return false;
    }

    return true;
}

std::unique_ptr<Model> FBXLoader::LoadModel(ID3D12Device* device, const std::string& filename)
{
    // Check if FBX SDK is initialized
    if (!m_fbxManager || !m_fbxImporter || !m_fbxIOSettings)
    {
        std::cerr << "FBX SDK not initialized" << std::endl;
        return nullptr;
    }

    // Initialize the importer
    if (!m_fbxImporter->Initialize(filename.c_str(), -1, m_fbxManager->GetIOSettings()))
    {
        std::cerr << "Failed to initialize FBX importer: " << m_fbxImporter->GetStatus().GetErrorString() << std::endl;
        return nullptr;
    }

    // Create a new scene so it can be populated by the imported file
    FbxScene* scene = FbxScene::Create(m_fbxManager, "ImportScene");
    if (!scene)
    {
        std::cerr << "Failed to create FBX scene" << std::endl;
        return nullptr;
    }

    // Import the contents of the file into the scene
    if (!m_fbxImporter->Import(scene))
    {
        std::cerr << "Failed to import FBX scene: " << m_fbxImporter->GetStatus().GetErrorString() << std::endl;
        scene->Destroy();
        return nullptr;
    }

    // Convert the scene to DirectX coordinate system if needed
    FbxAxisSystem::DirectX.ConvertScene(scene);

    // Convert the scene's units to meters if needed
    FbxSystemUnit::m.ConvertScene(scene);

    // Create a new model
    std::unique_ptr<Model> model = std::make_unique<Model>(device);

    // Process the scene
    if (!ProcessScene(device, scene, model.get()))
    {
        std::cerr << "Failed to process FBX scene" << std::endl;
        scene->Destroy();
        return nullptr;
    }

    // Process animations
    ProcessAnimations(scene, model.get());

    // Initialize the model
    if (!model->Initialize())
    {
        std::cerr << "Failed to initialize model" << std::endl;
        scene->Destroy();
        return nullptr;
    }

    // Clean up the scene
    scene->Destroy();

    return model;
}

bool FBXLoader::ProcessScene(ID3D12Device* device, FbxScene* scene, Model* model)
{
    // Get the root node of the scene
    FbxNode* rootNode = scene->GetRootNode();
    if (!rootNode)
    {
        std::cerr << "Failed to get root node from FBX scene" << std::endl;
        return false;
    }

    // Process all child nodes recursively
    for (int i = 0; i < rootNode->GetChildCount(); ++i)
    {
        FbxNode* childNode = rootNode->GetChild(i);

        // Process only mesh nodes
        if (childNode->GetNodeAttribute() &&
            childNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            if (!ProcessMesh(device, childNode, model))
            {
                std::cerr << "Failed to process mesh: " << childNode->GetName() << std::endl;
                return false;
            }
        }
    }

    return true;
}

bool FBXLoader::ProcessMesh(ID3D12Device* device, FbxNode* node, Model* model)
{
    FbxMesh* mesh = node->GetMesh();
    if (!mesh)
    {
        std::cerr << "Failed to get mesh from node: " << node->GetName() << std::endl;
        return false;
    }

    // Process the skeleton if the mesh has skinning information
    std::shared_ptr<Skeleton> skeleton = nullptr;
    if (mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
    {
        skeleton = ProcessSkeleton(mesh);
        model->SetSkeleton(skeleton);
    }

    // Get the number of vertices and polygons
    int vertexCount = mesh->GetControlPointsCount();
    int polygonCount = mesh->GetPolygonCount();

    // Get the direct access to the control points array
    FbxVector4* controlPoints = mesh->GetControlPoints();

    // Temporary vectors for vertex data
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;

    // Get element access for normals and UVs
    FbxGeometryElementNormal* normalElement = mesh->GetElementNormal();
    FbxGeometryElementUV* uvElement = mesh->GetElementUV();

    // Process materials
    int materialCount = node->GetMaterialCount();
    std::vector<std::shared_ptr<Material>> materials;

    for (int i = 0; i < materialCount; ++i)
    {
        FbxSurfaceMaterial* fbxMaterial = node->GetMaterial(i);
        std::shared_ptr<Material> material = CreateMaterialFromFbxProperties(device, fbxMaterial);
        materials.push_back(material);
    }

    // If no materials are defined, create a default one
    if (materials.empty())
    {
        std::shared_ptr<Material> defaultMaterial = std::make_shared<Material>(device);
        defaultMaterial->Initialize();
        materials.push_back(defaultMaterial);
    }

    // Create submeshes based on materials
    std::vector<SubMesh> submeshes(materials.size());
    for (size_t i = 0; i < submeshes.size(); ++i)
    {
        submeshes[i].Material = materials[i];
        submeshes[i].StartIndexLocation = 0;
        submeshes[i].IndexCount = 0;
        XMStoreFloat4x4(&submeshes[i].LocalTransform, XMMatrixIdentity());
    }

    // Process polygons and create vertices and indices
    int vertexOffset = 0;
    for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
    {
        // Get the material for this polygon
        int materialIndex = 0;
        if (mesh->GetElementMaterial())
        {
            switch (mesh->GetElementMaterial()->GetMappingMode())
            {
            case FbxGeometryElement::eByPolygon:
                materialIndex = mesh->GetElementMaterial()->GetIndexArray().GetAt(polygonIndex);
                break;
            default:
                // Just use the first material if not by polygon
                break;
            }
        }

        // Make sure material index is in range
        materialIndex = std::min(materialIndex, static_cast<int>(materials.size()) - 1);

        // Get the number of vertices in the polygon (usually 3 for triangles)
        int polygonSize = mesh->GetPolygonSize(polygonIndex);

        // Triangulate polygons with more than 3 vertices
        for (int i = 2; i < polygonSize; ++i)
        {
            // Create a triangle: 0, i-1, i
            for (int j = 0; j < 3; ++j)
            {
                int vertexIndex = (j == 0) ? 0 : ((j == 1) ? (i - 1) : i);
                int controlPointIndex = mesh->GetPolygonVertex(polygonIndex, vertexIndex);

                // Create a new vertex
                ModelVertex vertex;

                // Position
                FbxVector4 position = controlPoints[controlPointIndex];
                vertex.Position.x = static_cast<float>(position[0]);
                vertex.Position.y = static_cast<float>(position[1]);
                vertex.Position.z = static_cast<float>(position[2]);

                // Normal
                if (normalElement)
                {
                    FbxVector4 normal;

                    switch (normalElement->GetMappingMode())
                    {
                    case FbxGeometryElement::eByControlPoint:
                    {
                        int normalIndex = (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect) ?
                            controlPointIndex : normalElement->GetIndexArray().GetAt(controlPointIndex);
                        normal = normalElement->GetDirectArray().GetAt(normalIndex);
                        break;
                    }
                    case FbxGeometryElement::eByPolygonVertex:
                    {
                        int normalIndex = (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect) ?
                            vertexOffset + vertexIndex : normalElement->GetIndexArray().GetAt(vertexOffset + vertexIndex);
                        normal = normalElement->GetDirectArray().GetAt(normalIndex);
                        break;
                    }
                    }

                    vertex.Normal.x = static_cast<float>(normal[0]);
                    vertex.Normal.y = static_cast<float>(normal[1]);
                    vertex.Normal.z = static_cast<float>(normal[2]);
                }
                else
                {
                    // Default normal if not provided
                    vertex.Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                }

                // UV coordinates
                if (uvElement)
                {
                    FbxVector2 uv;

                    switch (uvElement->GetMappingMode())
                    {
                    case FbxGeometryElement::eByControlPoint:
                    {
                        int uvIndex = (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect) ?
                            controlPointIndex : uvElement->GetIndexArray().GetAt(controlPointIndex);
                        uv = uvElement->GetDirectArray().GetAt(uvIndex);
                        break;
                    }
                    case FbxGeometryElement::eByPolygonVertex:
                    {
                        int uvIndex = mesh->GetTextureUVIndex(polygonIndex, vertexIndex);
                        if (uvElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                        {
                            uvIndex = uvElement->GetIndexArray().GetAt(uvIndex);
                        }
                        uv = uvElement->GetDirectArray().GetAt(uvIndex);
                        break;
                    }
                    }

                    vertex.TexCoord.x = static_cast<float>(uv[0]);
                    vertex.TexCoord.y = 1.0f - static_cast<float>(uv[1]); // Flip V for DirectX
                }
                else
                {
                    // Default UV if not provided
                    vertex.TexCoord = XMFLOAT2(0.0f, 0.0f);
                }

                // Default bone weights and indices
                vertex.BoneWeights = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
                vertex.BoneIndices = XMUINT4(0, 0, 0, 0);

                // Add skinning information if available
                if (skeleton)
                {
                    // TODO: Process bone weights and indices
                    // This would require iterating through the skin deformers and collecting weights
                    // for this control point. For simplicity, we're using the default values.
                }

                // Add the vertex to the list
                vertices.push_back(vertex);

                // Add the index
                indices.push_back(static_cast<uint32_t>(vertices.size() - 1));

                // Update the submesh
                submeshes[materialIndex].IndexCount++;
            }
        }

        vertexOffset += polygonSize;
    }

    // Update submesh start indices
    UINT currentIndex = 0;
    for (size_t i = 0; i < submeshes.size(); ++i)
    {
        submeshes[i].StartIndexLocation = currentIndex;
        currentIndex += submeshes[i].IndexCount;
    }

    // Apply the node's transformation
    FbxAMatrix nodeTransform = node->EvaluateGlobalTransform();
    XMFLOAT4X4 transform = ConvertFbxMatrix(nodeTransform);
    model->SetGlobalTransform(transform);

    // Add the submeshes to the model
    for (const SubMesh& submesh : submeshes)
    {
        if (submesh.IndexCount > 0)
        {
            model->GetSubMeshes().push_back(submesh);
        }
    }

    // Set the vertex and index data on the model
    model->GetVertices().assign(vertices.begin(), vertices.end());
    model->GetIndices().assign(indices.begin(), indices.end());

    return true;
}

std::shared_ptr<Skeleton> FBXLoader::ProcessSkeleton(FbxMesh* mesh)
{
    std::shared_ptr<Skeleton> skeleton = std::make_shared<Skeleton>();

    // Count the number of skin deformers
    int deformerCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    if (deformerCount == 0)
    {
        return skeleton;
    }

    // We'll only process the first skin deformer for simplicity
    FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
    if (!skin)
    {
        return skeleton;
    }

    // Get the number of clusters (bones/joints)
    int clusterCount = skin->GetClusterCount();

    // First pass: create the bone hierarchy
    for (int i = 0; i < clusterCount; ++i)
    {
        FbxCluster* cluster = skin->GetCluster(i);
        FbxNode* boneNode = cluster->GetLink();

        if (!boneNode)
        {
            continue;
        }

        // Get the bone name
        std::string boneName = boneNode->GetName();

        // Get the parent bone
        FbxNode* parentNode = boneNode->GetParent();
        int parentIndex = -1;

        if (parentNode && parentNode != mesh->GetScene()->GetRootNode())
        {
            // Find the parent bone index
            std::string parentName = parentNode->GetName();
            parentIndex = skeleton->FindBoneIndex(parentName);
        }

        // Get the bind pose inverse matrix
        FbxAMatrix transformMatrix;
        cluster->GetTransformLinkMatrix(transformMatrix);

        // Convert to DirectX matrix
        XMFLOAT4X4 offsetMatrix = ConvertFbxMatrix(transformMatrix.Inverse());

        // Add the bone to the skeleton
        skeleton->AddBone(boneName, parentIndex, offsetMatrix);
    }

    // Update global transforms
    skeleton->UpdateGlobalTransforms();

    return skeleton;
}

void FBXLoader::ProcessAnimations(FbxScene* scene, Model* model)
{
    // Get the skeleton
    std::shared_ptr<Skeleton> skeleton = model->GetSkeleton();
    if (!skeleton)
    {
        return;
    }

    // Get animation stack count
    int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();

    for (int stackIndex = 0; stackIndex < animStackCount; ++stackIndex)
    {
        // Get the animation stack
        FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(stackIndex);

        // Set this animation stack as the active one
        scene->SetCurrentAnimationStack(animStack);

        // Get the animation name
        std::string animName = animStack->GetName();

        // Get the animation time span
        FbxTimeSpan timeSpan = animStack->GetLocalTimeSpan();
        FbxTime start = timeSpan.GetStart();
        FbxTime end = timeSpan.GetStop();

        // Convert to seconds
        float startTime = static_cast<float>(start.GetSecondDouble());
        float endTime = static_cast<float>(end.GetSecondDouble());
        float duration = endTime - startTime;

        // Create a new animation
        std::shared_ptr<Animation> animation = std::make_shared<Animation>(animName, duration);

        // Process all bones to create animation channels
        for (size_t boneIndex = 0; boneIndex < skeleton->GetBoneCount(); ++boneIndex)
        {
            const Bone& bone = skeleton->GetBones()[boneIndex];

            // Find the node for this bone
            FbxNode* boneNode = scene->FindNodeByName(bone.Name.c_str());
            if (!boneNode)
            {
                continue;
            }

            // Create a new animation channel
            AnimationChannel channel;
            channel.BoneName = bone.Name;

            // The number of keyframes to sample (24 fps is a common frame rate)
            const int framesPerSecond = 24;
            const int keyframeCount = static_cast<int>(duration * framesPerSecond);

            // Sample the animation at regular intervals
            for (int frame = 0; frame <= keyframeCount; ++frame)
            {
                float time = startTime + (frame * duration / keyframeCount);
                FbxTime fbxTime;
                fbxTime.SetSecondDouble(time);

                // Get the bone's transformation at this time
                FbxAMatrix transform = boneNode->EvaluateGlobalTransform(fbxTime);

                // Extract translation, rotation, and scale
                FbxVector4 translation = transform.GetT();
                FbxQuaternion rotation = transform.GetQ();
                FbxVector4 scale = transform.GetS();

                // Create a keyframe
                Keyframe keyframe;
                keyframe.Time = time - startTime; // Make time relative to animation start

                // Set translation
                keyframe.Translation.x = static_cast<float>(translation[0]);
                keyframe.Translation.y = static_cast<float>(translation[1]);
                keyframe.Translation.z = static_cast<float>(translation[2]);

                // Set rotation
                keyframe.Rotation.x = static_cast<float>(rotation[0]);
                keyframe.Rotation.y = static_cast<float>(rotation[1]);
                keyframe.Rotation.z = static_cast<float>(rotation[2]);
                keyframe.Rotation.w = static_cast<float>(rotation[3]);

                // Set scale
                keyframe.Scale.x = static_cast<float>(scale[0]);
                keyframe.Scale.y = static_cast<float>(scale[1]);
                keyframe.Scale.z = static_cast<float>(scale[2]);

                // Add the keyframe to the channel
                channel.Keyframes.push_back(keyframe);
            }

            // Add the channel to the animation
            if (!channel.Keyframes.empty())
            {
                animation->AddChannel(channel);
            }
        }

        // Add the animation to the model
        model->GetAnimations().push_back(animation);
    }
}

XMFLOAT4X4 FBXLoader::ConvertFbxMatrix(const FbxAMatrix& matrix)
{
    XMFLOAT4X4 result;

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            result.m[row][col] = static_cast<float>(matrix.Get(row, col));
        }
    }

    return result;
}

std::shared_ptr<Material> FBXLoader::CreateMaterialFromFbxProperties(ID3D12Device* device, FbxSurfaceMaterial* fbxMaterial)
{
    std::shared_ptr<Material> material = std::make_shared<Material>(device);
    material->Initialize();

    if (!fbxMaterial)
    {
        return material;
    }

    // Get material properties
    MaterialConstants& props = material->GetProperties();

    // Diffuse color
    FbxProperty diffuseProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
    if (diffuseProperty.IsValid())
    {
        FbxDouble3 diffuse = diffuseProperty.Get<FbxDouble3>();
        props.Albedo.x = static_cast<float>(diffuse[0]);
        props.Albedo.y = static_cast<float>(diffuse[1]);
        props.Albedo.z = static_cast<float>(diffuse[2]);
    }

    // Diffuse factor
    FbxProperty diffuseFactorProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
    if (diffuseFactorProperty.IsValid())
    {
        double diffuseFactor = diffuseFactorProperty.Get<FbxDouble>();
        props.Albedo.x *= static_cast<float>(diffuseFactor);
        props.Albedo.y *= static_cast<float>(diffuseFactor);
        props.Albedo.z *= static_cast<float>(diffuseFactor);
    }

    // Transparency
    FbxProperty transparencyProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sTransparencyFactor);
    if (transparencyProperty.IsValid())
    {
        double transparency = transparencyProperty.Get<FbxDouble>();
        props.Albedo.w = static_cast<float>(1.0 - transparency);
    }

    // Emissive color
    FbxProperty emissiveProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sEmissive);
    if (emissiveProperty.IsValid())
    {
        FbxDouble3 emissive = emissiveProperty.Get<FbxDouble3>();
        props.Emission.x = static_cast<float>(emissive[0]);
        props.Emission.y = static_cast<float>(emissive[1]);
        props.Emission.z = static_cast<float>(emissive[2]);
    }

    // Emissive factor
    FbxProperty emissiveFactorProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sEmissiveFactor);
    if (emissiveFactorProperty.IsValid())
    {
        double emissiveFactor = emissiveFactorProperty.Get<FbxDouble>();
        props.Emission.w = static_cast<float>(emissiveFactor);
    }

    // Specular (use for metallic/roughness approximation)
    FbxProperty specularProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sSpecular);
    if (specularProperty.IsValid())
    {
        FbxDouble3 specular = specularProperty.Get<FbxDouble3>();
        float avgSpecular = (static_cast<float>(specular[0]) +
            static_cast<float>(specular[1]) +
            static_cast<float>(specular[2])) / 3.0f;
        props.Metallic = avgSpecular;
    }

    // Shininess (use for roughness approximation)
    FbxProperty shininessProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
    if (shininessProperty.IsValid())
    {
        double shininess = shininessProperty.Get<FbxDouble>();
        // Convert shininess to roughness (this is an approximation)
        props.Roughness = 1.0f - static_cast<float>(std::min(shininess / 100.0, 1.0));
    }

    // TODO: Load textures
    // For simplicity, we're not loading textures in this example,
    // but you would use the FbxProperty::GetSrcObject<FbxTexture>() function
    // to get texture objects and extract filenames.

    return material;
}

std::shared_ptr<class DXTexture> FBXLoader::LoadTexture(ID3D12Device* device, const std::string& filename)
{
    // This is a placeholder - you would implement this to load textures
    // using your DXTexture class

    // For now, return nullptr
    return nullptr;
}