#include"BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION, float4 normal:NORMAL,float2 uv : TEXCOORD,min16uint2 boneno:BONE_NO,min16uint weight :WEIGHT)
{
    Output output; //�s�N�Z���V�F�[�_�[�ɂ킽���l
    output.svpos = mul(mat,pos);
    output.normal = normal;
    output.uv = uv;
    return output;

}