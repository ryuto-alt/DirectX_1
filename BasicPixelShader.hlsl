#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
    // �@���𐳋K��
    float3 normal = normalize(input.normal.xyz);

    // ����
    float ambientStrength = 0.2;
    float3 ambient = ambientStrength * float3(1.0, 1.0, 1.0);

    // ���s����
    float3 lightDir = normalize(float3(0.5, -1.0, -0.5)); // ���C�g�̕���
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * float3(1.0, 1.0, 1.0); // ���F�̌�

    // �����F
    float3 resultColor = ambient + diffuse;

    return float4(resultColor, 1.0); // RGB+Alpha
}