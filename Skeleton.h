#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <unordered_map>

struct Bone {
    std::string Name;
    int ParentIndex;                   // Index of the parent bone (-1 for root)
    DirectX::XMFLOAT4X4 OffsetMatrix;  // Bind pose inverse matrix
    DirectX::XMFLOAT4X4 LocalTransform; // Current local transform
    DirectX::XMFLOAT4X4 GlobalTransform; // Current global transform
};

class Skeleton {
public:
    Skeleton();
    ~Skeleton();

    // Add a bone to the skeleton
    int AddBone(const std::string& name, int parentIndex, const DirectX::XMFLOAT4X4& offsetMatrix);

    // Find a bone by name
    int FindBoneIndex(const std::string& name) const;

    // Update global transforms for all bones
    void UpdateGlobalTransforms();

    // Get final transforms for shader use (global * offset)
    std::vector<DirectX::XMFLOAT4X4> GetFinalTransforms() const;

    // Getters
    const std::vector<Bone>& GetBones() const { return m_bones; }
    size_t GetBoneCount() const { return m_bones.size(); }

    // Apply bone transformations
    void SetBoneTransform(int boneIndex, const DirectX::XMFLOAT4X4& transform);
    void SetBoneTransform(const std::string& name, const DirectX::XMFLOAT4X4& transform);

private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int> m_boneNameToIndex;
};