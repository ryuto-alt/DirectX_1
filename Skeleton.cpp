#include "Skeleton.h"

using namespace DirectX;

Skeleton::Skeleton()
{
}

Skeleton::~Skeleton()
{
}

int Skeleton::AddBone(const std::string& name, int parentIndex, const XMFLOAT4X4& offsetMatrix)
{
    // Check if a bone with this name already exists
    auto it = m_boneNameToIndex.find(name);
    if (it != m_boneNameToIndex.end())
    {
        return it->second;
    }

    // Create a new bone
    Bone bone;
    bone.Name = name;
    bone.ParentIndex = parentIndex;
    bone.OffsetMatrix = offsetMatrix;

    // Initialize transforms to identity
    XMStoreFloat4x4(&bone.LocalTransform, XMMatrixIdentity());
    XMStoreFloat4x4(&bone.GlobalTransform, XMMatrixIdentity());

    // Add the bone to the skeleton
    int boneIndex = static_cast<int>(m_bones.size());
    m_bones.push_back(bone);
    m_boneNameToIndex[name] = boneIndex;

    return boneIndex;
}

int Skeleton::FindBoneIndex(const std::string& name) const
{
    auto it = m_boneNameToIndex.find(name);
    if (it != m_boneNameToIndex.end())
    {
        return it->second;
    }

    return -1;  // Bone not found
}

void Skeleton::UpdateGlobalTransforms()
{
    // Calculate global transforms for each bone
    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        Bone& bone = m_bones[i];

        if (bone.ParentIndex == -1)
        {
            // Root bone, global transform equals local transform
            bone.GlobalTransform = bone.LocalTransform;
        }
        else
        {
            // Child bone, global transform is parent's global transform * local transform
            XMMATRIX parentGlobal = XMLoadFloat4x4(&m_bones[bone.ParentIndex].GlobalTransform);
            XMMATRIX localTransform = XMLoadFloat4x4(&bone.LocalTransform);
            XMMATRIX globalTransform = XMMatrixMultiply(localTransform, parentGlobal);
            XMStoreFloat4x4(&bone.GlobalTransform, globalTransform);
        }
    }
}

std::vector<XMFLOAT4X4> Skeleton::GetFinalTransforms() const
{
    std::vector<XMFLOAT4X4> finalTransforms(m_bones.size());

    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        const Bone& bone = m_bones[i];

        // Final transform = Global transform * Offset matrix
        XMMATRIX globalTransform = XMLoadFloat4x4(&bone.GlobalTransform);
        XMMATRIX offsetMatrix = XMLoadFloat4x4(&bone.OffsetMatrix);
        XMMATRIX finalTransform = XMMatrixMultiply(offsetMatrix, globalTransform);

        XMStoreFloat4x4(&finalTransforms[i], finalTransform);
    }

    return finalTransforms;
}

void Skeleton::SetBoneTransform(int boneIndex, const XMFLOAT4X4& transform)
{
    if (boneIndex >= 0 && boneIndex < static_cast<int>(m_bones.size()))
    {
        m_bones[boneIndex].LocalTransform = transform;
    }
}

void Skeleton::SetBoneTransform(const std::string& name, const XMFLOAT4X4& transform)
{
    int boneIndex = FindBoneIndex(name);
    if (boneIndex >= 0)
    {
        SetBoneTransform(boneIndex, transform);
    }
}