#include "Animation.h"
#include "Skeleton.h"

using namespace DirectX;

Animation::Animation(const std::string& name, float duration)
    : m_name(name), m_duration(duration)
{
}

Animation::~Animation()
{
}

void Animation::AddChannel(const AnimationChannel& channel)
{
    // Store the channel index for quick lookup by bone name
    m_boneNameToChannelIndex[channel.BoneName] = static_cast<int>(m_channels.size());
    m_channels.push_back(channel);
}

void Animation::Apply(std::shared_ptr<Skeleton> skeleton, float time, bool loop)
{
    if (!skeleton || m_channels.empty() || m_duration <= 0.0f)
    {
        return;
    }

    // Handle looping
    if (loop)
    {
        time = fmodf(time, m_duration);
        if (time < 0.0f)
        {
            time += m_duration;
        }
    }
    else
    {
        // Clamp time to animation duration
        time = std::max(0.0f, std::min(time, m_duration));
    }

    // Apply each animation channel to the corresponding bone
    for (const auto& channel : m_channels)
    {
        int boneIndex = skeleton->FindBoneIndex(channel.BoneName);
        if (boneIndex < 0 || channel.Keyframes.empty())
        {
            continue;  // Skip if bone not found or no keyframes
        }

        // Find the keyframes to interpolate between
        const auto& keyframes = channel.Keyframes;

        if (keyframes.size() == 1 || time <= keyframes.front().Time)
        {
            // Use the first keyframe if time is before or at the first keyframe
            XMFLOAT4X4 transform = KeyframeToMatrix(keyframes.front());
            skeleton->SetBoneTransform(boneIndex, transform);
            continue;
        }

        if (time >= keyframes.back().Time)
        {
            // Use the last keyframe if time is after or at the last keyframe
            XMFLOAT4X4 transform = KeyframeToMatrix(keyframes.back());
            skeleton->SetBoneTransform(boneIndex, transform);
            continue;
        }

        // Find the two keyframes to interpolate between
        size_t nextKeyframeIndex = 0;
        for (size_t i = 0; i < keyframes.size() - 1; ++i)
        {
            if (time >= keyframes[i].Time && time < keyframes[i + 1].Time)
            {
                nextKeyframeIndex = i + 1;
                break;
            }
        }

        size_t prevKeyframeIndex = nextKeyframeIndex - 1;
        const Keyframe& prevKeyframe = keyframes[prevKeyframeIndex];
        const Keyframe& nextKeyframe = keyframes[nextKeyframeIndex];

        // Calculate interpolation factor
        float keyframeDuration = nextKeyframe.Time - prevKeyframe.Time;
        float factor = (time - prevKeyframe.Time) / keyframeDuration;

        // Interpolate between keyframes
        Keyframe interpolatedKeyframe = InterpolateKeyframes(prevKeyframe, nextKeyframe, factor);
        XMFLOAT4X4 transform = KeyframeToMatrix(interpolatedKeyframe);

        // Apply the transform to the bone
        skeleton->SetBoneTransform(boneIndex, transform);
    }

    // Update the global transforms after setting all bone transforms
    skeleton->UpdateGlobalTransforms();
}

Keyframe Animation::InterpolateKeyframes(const Keyframe& a, const Keyframe& b, float t)
{
    Keyframe result;

    // Linear interpolation for time and translation
    result.Time = a.Time + t * (b.Time - a.Time);

    // Linear interpolation for translation
    result.Translation = XMFLOAT3(
        a.Translation.x + t * (b.Translation.x - a.Translation.x),
        a.Translation.y + t * (b.Translation.y - a.Translation.y),
        a.Translation.z + t * (b.Translation.z - a.Translation.z)
    );

    // Spherical linear interpolation (SLERP) for rotation
    XMVECTOR quatA = XMLoadFloat4(&a.Rotation);
    XMVECTOR quatB = XMLoadFloat4(&b.Rotation);
    XMVECTOR quatResult = XMQuaternionSlerp(quatA, quatB, t);
    XMStoreFloat4(&result.Rotation, quatResult);

    // Linear interpolation for scale
    result.Scale = XMFLOAT3(
        a.Scale.x + t * (b.Scale.x - a.Scale.x),
        a.Scale.y + t * (b.Scale.y - a.Scale.y),
        a.Scale.z + t * (b.Scale.z - a.Scale.z)
    );

    return result;
}

XMFLOAT4X4 Animation::KeyframeToMatrix(const Keyframe& keyframe)
{
    // Create transformation matrices
    XMMATRIX translation = XMMatrixTranslation(
        keyframe.Translation.x,
        keyframe.Translation.y,
        keyframe.Translation.z
    );

    XMMATRIX rotation = XMMatrixRotationQuaternion(
        XMLoadFloat4(&keyframe.Rotation)
    );

    XMMATRIX scale = XMMatrixScaling(
        keyframe.Scale.x,
        keyframe.Scale.y,
        keyframe.Scale.z
    );

    // Combine transformations: Scale -> Rotate -> Translate
    XMMATRIX transform = XMMatrixMultiply(XMMatrixMultiply(scale, rotation), translation);

    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, transform);

    return result;
}