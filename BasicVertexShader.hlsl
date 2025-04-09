#include "BasicShaderHeader.hlsli"

Output BasicVS(float3 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    output.svpos = mul(mat, float4(pos, 1.0f));
    output.uv = uv;
    return output;
}
