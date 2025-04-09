#include "BasicShaderHeader.hlsli"

Output BasicVS(float3 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    // ここを変更：行列を左側に置いて mul(mat, float4(...)) とする
    output.svpos = mul(mat, float4(pos, 1.0f));
    output.uv = uv;
    return output;
}
