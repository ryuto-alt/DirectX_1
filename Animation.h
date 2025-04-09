#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class Skeleton;

// A keyframe stores a bone transformation at a specific time
struct Keyframe {
    float Time;                       // Time of the keyframe
    DirectX::XMFLOAT3 Translation;    // Position
    DirectX::XMFLOAT4 Rotation;       // Quaternion rotation
    DirectX::XMFLOAT3 Scale;          // Scale
};

// An animation channel targets a specific bone
struct AnimationChannel {
    std::string BoneName;             // Name of the bone this channel affects
    std::vector<Keyframe> Keyframes;  // Keyframes for this bone
};

class Animation {
public:
    Animation(const std::string& name, float duration);
    ~Animation();

    // Add a channel to the animation
    void AddChannel(const AnimationChannel& channel);

    // Apply the animation to a skeleton at a specific time
    void Apply(std::shared_ptr<Skeleton> skeleton, float time, bool loop = true);

    // Getters
    const std::string& GetName() const { return m_name; }
    float GetDuration() const { return m_duration; }
    const std::vector<AnimationChannel>& GetChannels() const { return m_channels; }

private:
    // Helper function to interpolate between keyframes
    Keyframe InterpolateKeyframes(const Keyframe& a, const Keyframe& b, float t);

    // Convert keyframe to transformation matrix
    DirectX::XMFLOAT4X4 KeyframeToMatrix(const Keyframe& keyframe);

    std::string m_name;
    float m_duration;
    std::vector<AnimationChannel> m_channels;

    // Cache bone indices for quicker access
    std::unordered_map<std::string, int> m_boneNameToChannelIndex;
};