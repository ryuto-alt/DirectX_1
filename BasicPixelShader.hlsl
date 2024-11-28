#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
    // 法線を正規化
    float3 normal = normalize(input.normal.xyz);

    // 環境光
    float ambientStrength = 0.2;
    float3 ambient = ambientStrength * float3(1.0, 1.0, 1.0);

    // 平行光源
    float3 lightDir = normalize(float3(0.5, -1.0, -0.5)); // ライトの方向
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * float3(1.0, 1.0, 1.0); // 白色の光

    // 合成色
    float3 resultColor = ambient + diffuse;

    return float4(resultColor, 1.0); // RGB+Alpha
}