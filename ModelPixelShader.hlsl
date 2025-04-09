#include "ModelShaderHeader.hlsli"

// PBR lighting model
float4 ModelPS(VSOutput input) : SV_TARGET
{
    // Sample textures
    float4 albedoTexValue = AlbedoTexture.Sample(DefaultSampler, input.TexCoord);
    float4 emissiveTexValue = EmissiveTexture.Sample(DefaultSampler, input.TexCoord);
    float4 metallicRoughnessTexValue = MetallicRoughnessTexture.Sample(DefaultSampler, input.TexCoord);
    float occlusionTexValue = OcclusionTexture.Sample(DefaultSampler, input.TexCoord).r;
    
    // Combine material properties with texture values
    float4 albedo = Material.Albedo * albedoTexValue;
    float4 emission = Material.Emission * emissiveTexValue;
    float metallic = Material.Metallic * metallicRoughnessTexValue.b;
    float roughness = Material.Roughness * metallicRoughnessTexValue.g;
    float ao = Material.AmbientOcclusion * occlusionTexValue;
    
    // Normalize input normal
    float3 N = normalize(input.Normal);
    
    // Calculate view direction
    float3 V = normalize(CameraPosition.xyz - input.WorldPosition);
    
    // Define light direction (directional light for simplicity)
    float3 L = normalize(float3(1, 1, 1));
    
    // Calculate half vector
    float3 H = normalize(L + V);
    
    // Calculate dot products for lighting equations
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    // Constants
    const float PI = 3.14159265359;
    
    // Calculate the specular power based on roughness
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    
    // Fresnel reflectance at normal incidence (F0)
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo.rgb, metallic);
    
    // Cook-Torrance BRDF
    
    // Distribution term (D) - GGX/Trowbridge-Reitz
    float D = alpha2 / (PI * pow(NdotH * NdotH * (alpha2 - 1.0) + 1.0, 2.0));
    
    // Geometric Attenuation (G) - Smith's method with Schlick-GGX
    float k = pow(roughness + 1.0, 2.0) / 8.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G = G1L * G1V;
    
    // Fresnel term (F) - Schlick's approximation
    float3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
    
    // Specular BRDF
    float3 specular = (D * G * F) / (4.0 * NdotL * NdotV + 0.001);
    
    // Diffuse BRDF (Lambert)
    float3 diffuse = albedo.rgb * (1.0 - metallic) / PI;
    
    // Combine diffuse and specular
    float3 directLight = (diffuse + specular) * NdotL;
    
    // Ambient lighting
    float3 ambient = albedo.rgb * ao * 0.2;
    
    // Emissive contribution
    float3 emissive = emission.rgb * emission.a;
    
    // Final color
    float3 finalColor = ambient + directLight + emissive;
    
    // Apply basic tone mapping (Reinhard)
    finalColor = finalColor / (finalColor + float3(1.0, 1.0, 1.0));
    
    // Gamma correction (assume output is in sRGB space)
    finalColor = pow(finalColor, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    
    return float4(finalColor, albedo.a);
}