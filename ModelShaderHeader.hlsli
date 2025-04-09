// Constants and structures for model rendering

// Maximum number of bones supported
#define MAX_BONES 128

// Output structure for the vertex shader
struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

// Material properties
struct MaterialData
{
    float4 Albedo; // Base color (RGB) and opacity (A)
    float4 Emission; // Emissive color (RGB) and emission strength (A)
    float Metallic; // Metallic factor (0=dielectric, 1=metal)
    float Roughness; // Surface roughness
    float AmbientOcclusion; // Ambient occlusion factor
    float Padding; // Padding for 16-byte alignment
};

// Transform matrices
cbuffer ModelTransforms : register(b0)
{
    float4x4 Model; // Model to world transform
    float4x4 View; // World to view transform
    float4x4 Projection; // View to projection transform
    float4x4 MVP; // Combined model-view-projection matrix
    float4 CameraPosition; // Camera position in world space
}

// Material properties
cbuffer MaterialBuffer : register(b1)
{
    MaterialData Material;
}

// Bone transformations for skeletal animation
cbuffer BoneTransforms : register(b2)
{
    float4x4 BoneMatrices[MAX_BONES];
}

// Textures
Texture2D AlbedoTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D MetallicRoughnessTexture : register(t2);
Texture2D EmissiveTexture : register(t3);
Texture2D OcclusionTexture : register(t4);

// Samplers
SamplerState DefaultSampler : register(s0);