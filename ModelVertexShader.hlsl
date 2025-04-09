#include "ModelShaderHeader.hlsli"

// Input structure matching our ModelVertex
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 BoneWeights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
};

VSOutput ModelVS(VSInput input)
{
    VSOutput output;
    
    // Initialize with zero
    float4 position = float4(0, 0, 0, 0);
    float4 normal = float4(0, 0, 0, 0);
    
    // Apply skinning if weights are not zero
    if (any(input.BoneWeights))
    {
        // Compute skinned position and normal
        for (int i = 0; i < 4; ++i)
        {
            float weight = input.BoneWeights[i];
            if (weight > 0)
            {
                uint boneIndex = input.BoneIndices[i];
                float4x4 boneTransform = BoneMatrices[boneIndex];
                
                position += weight * mul(float4(input.Position, 1.0), boneTransform);
                normal += weight * mul(float4(input.Normal, 0.0), boneTransform);
            }
        }
    }
    else
    {
        // No skinning, just use the input position
        position = float4(input.Position, 1.0);
        normal = float4(input.Normal, 0.0);
    }
    
    // Transform position from model to world space
    float4 worldPosition = mul(position, Model);
    
    // Transform normal to world space
    float3 worldNormal = normalize(mul(normal.xyz, (float3x3) Model));
    
    // Transform position from world to clip space
    output.Position = mul(worldPosition, mul(View, Projection));
    
    // Pass through other values
    output.WorldPosition = worldPosition.xyz;
    output.Normal = worldNormal;
    output.TexCoord = input.TexCoord;
    
    return output;
}