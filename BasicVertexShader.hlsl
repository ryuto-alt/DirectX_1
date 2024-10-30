#include"BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output; //ピクセルシェーダーにわたす値
    output.svpos = pos;
    output.uv = uv;
    return output;

}